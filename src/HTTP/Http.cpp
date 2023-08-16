#include "Http.h"

const char *ok_200_title = "OK";
const char *error_400_title = "Bad Request";
const char *error_400_form = "Your request has bad syntax or is inherently impossible to staisfy.\n";
const char *error_403_title = "Forbidden";
const char *error_403_form = "You do not have permission to get file form this server.\n";
const char *error_404_title = "Not Found";
const char *error_404_form = "The requested file was not found on this server.\n";
const char *error_500_title = "Internal Error";
const char *error_500_form = "There was an unusual problem serving the request file.\n";

void Http::init(char *root)
{
    doc_path = root;
    m_epollfd = m_client->getEpollfd();
    m_utils->add_fd(m_epollfd, m_client->getFd(), true, m_client->getTrigMode());
    init();
}

void Http::init()
{
    m_check_state = CHECK_STATE_REQUESTLINE;
    m_check_idx = 0;
    m_start_line = 0;
    m_read_idx = 0;
    m_method = GET;
    isPost = false;
    m_url = 0;
    m_version = 0;
    m_keep = false;
    m_ok = false;
    m_content_length = 0;
    m_host = 0;
    m_write_idx = 0;
    m_bytes_have_send = 0;
    m_bytes_to_send = 0;

    bzero(m_write_buf, WRITE_BUF_SIZE);
    bzero(m_real_file, FILENAME_LEN);
    bzero(m_read_buf, READ_BUF_SIZE);
}

bool Http::read_from()
{
    if (m_read_idx >= READ_BUF_SIZE)
    {
        return false;
    }

    int bytes_read = 0;
    while (true)
    {
        bytes_read = recv(m_client->getFd(), m_read_buf + m_read_idx, READ_BUF_SIZE - m_read_idx, 0);
        if (bytes_read == -1)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                break;
            }
            return false;
        }
        else if (bytes_read == 0)
        {
            return false;
        }
        m_read_idx += bytes_read;
    }
    return true;
}

void Http::process()
{    
    HTTP_CODE read_ret = process_read();
    int sockfd = m_client->getFd();
    int trigMode = m_client->getTrigMode();

    if (read_ret == NO_REQUEST)
    {
        m_utils->mod_fd(m_epollfd, sockfd, EPOLLIN, trigMode);
        return;
    }

    bool write_ret = process_write(read_ret);
    if (!write_ret)
    {
        LOG_INFO(m_utils->getLogger()) << "process_write error";
        close_conn();
    }
    m_utils->mod_fd(m_epollfd, sockfd, EPOLLOUT, trigMode);
}

HTTP_CODE Http::process_read()
{
    HTTP_CODE ret = NO_REQUEST;
    LINE_STATUS line_state = LINE_OK;

    char *text = 0;
    while ((m_check_state == CHECK_STATE_CONTENT && line_state == LINE_OK) || (line_state = parse_line()) == LINE_OK)
    {
        text = get_line();
        m_start_line = m_check_idx;
        switch (m_check_state)
        {
        case CHECK_STATE_REQUESTLINE:
        {
            ret = parse_req_line(text);
            if (ret == BAD_REQUEST)
            {
                return BAD_REQUEST;
            }
            break;
        }

        case CHECK_STATE_HEADER:
        {
            ret = parse_req_header(text);
            if (ret == BAD_REQUEST)
                return BAD_REQUEST;
            else if (ret == GET_REQUEST)
            {
                return do_request();
            }
            break;
        }

        case CHECK_STATE_CONTENT:
        {
            ret = parse_req_content(text);
            if (ret == GET_REQUEST)
            {
                return do_request();
            }
            line_state = LINE_IMPERFECT;
            break;
        }
        default:
        {
            return INTERNAL_ERROR;
        }
        }
    }
    return NO_REQUEST;
}

HTTP_CODE Http::parse_req_line(char *req_text)
{
    // GET /index.html HTTP/1.1
    m_url = strpbrk(req_text, " \t"); // 返回空格或者回车符第一次出现的索引

    *m_url++ = '\0';         // GET\0/ HTTP/1.1
    char *method = req_text; // method = GET

    if (strcasecmp(method, "GET") == 0)
    { // 忽略大小写比较
        m_method = GET;
    }
    if (strcasecmp(method, "POST") == 0)
    {
        m_method = POST;
        isPost = true;
    }
    m_version = strpbrk(m_url, " \t"); // m_url = /index.html HTTP/1.1
    if (!m_version)
    {
        return BAD_REQUEST;
    }
    *m_version++ = '\0'; /// index.html\0HTTP/1.1
    // if(strcasecmp(m_version , "HTTP/1.1") != 0){
    //     return BAD_REQUEST;
    // }

    if (strncasecmp(m_url, "http://", 7) == 0)
    { // http://192.168.130.192/index.html
        m_url += 7;
        m_url = strchr(m_url, '/'); // /index.html
    }

    if (!m_url || (m_url[0] != '/'))
    {
        return BAD_REQUEST;
    }
    if (strlen(m_url) == 1)
    {
        strcat(m_url, "login.html");
    }
    m_check_state = CHECK_STATE_HEADER; // 主状态机状态变成检查请求头

    return NO_REQUEST; // 报文还没有解析完成
}

HTTP_CODE Http::parse_req_header(char *req_text)
{
    if (req_text[0] == '\0')
    { // 遇到空行,表示头部字段解析完成
        if (m_content_length != 0)
        { // m_content_length表示请求体长度，如果没有请求体，则不需要解析请求体，已经解析完成
            m_check_state = CHECK_STATE_CONTENT;
            return NO_REQUEST;
        }
        return GET_REQUEST; // 解析完成
    }
    else if (strncasecmp(req_text, "Connection:", 11) == 0)
    {
        req_text += 11;
        req_text += strspn(req_text, " \t"); // 返回不是 \t的下标
        if (strcasecmp(req_text, "keep-alive") == 0)
        {
            m_keep = true;
        }
        else
        {
            m_keep = false;
        }
    }
    else if (strncasecmp(req_text, "Host:", 5) == 0)
    {
        req_text += 5;
        req_text += strspn(req_text, " \t");
        m_host = req_text;
    }
    else if (strncasecmp(req_text, "Content-Length:", 15) == 0)
    {
        req_text += 15;
        req_text += strspn(req_text, " \t");
        m_content_length = atol(req_text);
    }
    else
    {
        // 不处理不需要信息
        // printf("unknow header %s\n",req_text);
    }
    return NO_REQUEST;
}

Http::Http()
{
    m_utils = Utils::getInstance();
}

Http::~Http()
{
}

HTTP_CODE Http::parse_req_content(char *req_text)
{
    if (m_read_idx >= (m_content_length + m_check_idx))
    {
        req_text[m_content_length] = '\0';
        m_data = req_text;
        return GET_REQUEST;
    }
    return NO_REQUEST;
}

LINE_STATUS Http::parse_line()
{
    char temp;
    for (; m_check_idx < m_read_idx; ++m_check_idx)
    {
        temp = m_read_buf[m_check_idx];
        if (temp == '\r')
        {
            if (m_check_idx + 1 == m_read_idx)
            {
                return LINE_IMPERFECT;
            }
            else if (m_read_buf[m_check_idx + 1] == '\n')
            {
                m_read_buf[m_check_idx++] = '\0';
                m_read_buf[m_check_idx++] = '\0';
                return LINE_OK;
            }
            return LINE_BAD;
        }
        else if (temp == '\n')
        {
            if (m_check_idx > 1 && m_read_buf[m_check_idx - 1] == '\r')
            {
                m_read_buf[m_check_idx - 1] = '\0';
                m_read_buf[m_check_idx++] = '\0';
                return LINE_OK;
            }
            return LINE_BAD;
        }
    }
    return LINE_IMPERFECT;
}

HTTP_CODE Http::do_request()
{
    strcpy(m_real_file, doc_path);
    int len = strlen(doc_path);
    const char *p = strrchr(m_url, '/');
    // post请求
    if (isPost && (*(p + 1) == '1' || *(p + 1) == '2'))
    {
        // userName=123&userPwd=123
        string userName;
        string userPwd;
        int i = 0;
        for (i = 9; m_data[i] != '&'; ++i)
        {
            userName += m_data[i];
        }
        for (i = i + 9; m_data[i] != '\0'; ++i)
        {
            userPwd += m_data[i];
        }

        if (userName[0] == '%')
        {
            userName = curl_unescape(userName.c_str(), 0);
        }

        // 注册
        if (*(p + 1) == '2')
        {
            if (m_utils->getUsers().find(userName) == m_utils->getUsers().end())
            {
                bool ret = m_utils->sqlInsertNP(userName, userPwd);
                if (ret)
                {
                    resonse_msg["code"] = "200";
                }
                else
                {
                    resonse_msg["code"] = "500";
                    resonse_msg["msg"] = string(error_500_title);
                }
            }
            else
            {
                resonse_msg["code"] = "500";
                resonse_msg["msg"] = "user have exist";
            }
        }
        // 登录
        else if (*(p + 1) == '1')
        {
            if (m_utils->getUsers().find(userName) != m_utils->getUsers().end()){
                if (m_utils->getUsers().at(userName) == userPwd)
                {
                    resonse_msg["code"] = "200";
                }
                else
                {
                    resonse_msg["code"] = "500";
                    resonse_msg["msg"] = "password error";
                }
            }
            else{
                resonse_msg["code"] = "500";
                resonse_msg["msg"] = "user not exist";  
            }
        }
        return LOG_REG_RESPONSE;
    }
    else
    {
        strncpy(m_real_file + len, m_url, FILENAME_LEN - len - 1);
    }
    if (stat(m_real_file, &m_file_stat) < 0)
    {
        return NO_RESOURSE;
    }

    // 判断访问权限
    if (!(m_file_stat.st_mode & S_IROTH))
    {
        return FORBIDDEN_REQUST;
    }

    // 判断是否是目录
    if (S_ISDIR(m_file_stat.st_mode))
    {
        return BAD_REQUEST;
    }

    // 以只读方式读取文件
    int fd = open(m_real_file, O_RDONLY);

    // 创建文件映射 PROT_READ页内容可以被读取  MAP_PRIVATE建立一个写入时拷贝的私有映射,内存区域的写入不会影响到原文件
    m_real_file_addr = (char *)mmap(0, m_file_stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);

    close(fd);
    return FILE_REQUEST;
}

void Http::unmap()
{
    if (m_real_file_addr)
    {
        munmap(m_real_file_addr, m_file_stat.st_size);
        m_real_file_addr = 0;
    }
}

void Http::setClient(Client *client)
{
    m_client = client;
}

bool Http::write_to()
{
    int temp = 0;
    int sockfd = m_client->getFd();
    int trigMode = m_client->getTrigMode();
    if (m_bytes_to_send == 0)
    {
        m_utils->mod_fd(m_epollfd, sockfd, EPOLLIN, trigMode);
        init();
        return true;
    }
    while (true)
    {
        temp = writev(sockfd, m_iv, m_iv_num);
        if (temp <= -1)
        {
            if (errno == EAGAIN)
            {
                m_utils->mod_fd(m_epollfd, sockfd, EPOLLOUT, trigMode);
                return true;
            }
            unmap();
            return false;
        }
        m_bytes_to_send -= temp;
        m_bytes_have_send += temp;
        if (m_bytes_have_send >= m_iv[0].iov_len)
        {
            // 头文件发送完
            m_iv[0].iov_len = 0;
            m_iv[1].iov_base = m_real_file_addr + (m_bytes_have_send - m_write_idx);
            m_iv[1].iov_len = m_bytes_to_send;
        }
        else
        {
            m_iv[0].iov_base = m_write_buf + m_bytes_have_send;
            m_iv[0].iov_len = m_iv[0].iov_len - temp;
        }
        if (m_bytes_to_send <= 0)
        {

            // 发送HTTP响应成功，根据HTTP请求中的Connection字段决定是否立即关闭连接
            unmap();
            m_utils->mod_fd(m_epollfd, sockfd, EPOLLIN, trigMode);
            if (m_keep)
            {
                init();
                return true;
            }
            else
            {
                return false;
            }
        }
    }
}

bool Http::add_response(const char *format, ...)
{
    if (m_write_idx >= WRITE_BUF_SIZE)
    {
        return false;
    }
    va_list arg_list;
    va_start(arg_list, format);
    int len = vsnprintf(m_write_buf + m_write_idx, WRITE_BUF_SIZE - 1 - m_write_idx, format, arg_list);
    if (len >= (WRITE_BUF_SIZE - 1 - m_write_idx))
    {
        return false;
    }
    m_write_idx += len;
    va_end(arg_list);
    return true;
}

bool Http::add_state_line(int state, const char *title)
{
    return add_response("%s %d %s\r\n", "HTTP/1.1", state, title);
}

bool Http::add_headers(int content_len)
{
    add_content_length(content_len);
    add_content_type();
    add_iskeep();
    add_blank_line();
    return true;
}

bool Http::add_content_length(int content_len)
{
    return add_response("Content-Length: %d\r\n", content_len);
}

bool Http::add_iskeep()
{
    return add_response("Connection: %s\r\n", (m_keep == true) ? "keep-alive" : "close");
}

bool Http::add_blank_line()
{
    return add_response("%s", "\r\n");
}

bool Http::add_content(const char *content)
{
    return add_response("%s", content);
}

bool Http::add_map_content()
{
    // 构造 JSON 字符串
    string content = "{";
    
    bool firstItem = true;
    for (const auto& item : resonse_msg) {
        if (!firstItem) {
            content += ", ";
        } else {
            firstItem = false;
        }
        content += "\"" + item.first + "\": \"" + item.second + "\"";
    }
    content += "}";
    add_headers(strlen(content.c_str()));
    return add_response("%s", content.c_str());
}

bool Http::add_content_type()
{
    return add_response("Content-Type:%s\r\n",isPost == true ? "application/json":"text/html");
}

bool Http::process_write(HTTP_CODE ret)
{
    switch (ret)
    {
    case INTERNAL_ERROR:
    {
        LOG_ERROR(m_utils->getLogger()) << "INTERNAL_ERROR";
        add_state_line(500, error_500_title);
        add_headers(strlen(error_500_form));
        if (!add_content(error_500_form))
        {
            return false;
        }
        break;
    }

    case BAD_REQUEST:
    {
        LOG_INFO(m_utils->getLogger()) << "BAD_REQUEST";
        add_state_line(400, error_400_title);
        add_headers(strlen(error_400_form));
        if (!add_content(error_400_form))
        {
            return false;
        }
        break;
    }
    case NO_RESOURSE:
    {
        LOG_INFO(m_utils->getLogger()) << "NO_RESOURSE";
        add_state_line(404, error_404_title);
        add_headers(strlen(error_404_form));
        if (!add_content(error_404_form))
        {
            return false;
        }
        break;
    }

    case FORBIDDEN_REQUST:
    {
        LOG_INFO(m_utils->getLogger()) << "FORBIDDEN_REQUST";
        add_state_line(403, error_403_title);
        add_headers(strlen(error_403_form));
        if (!add_content(error_403_form))
        {
            return false;
        }
        break;
    }

    case FILE_REQUEST:
    {
        add_state_line(200, ok_200_title);
        add_headers(m_file_stat.st_size);

        m_iv[0].iov_base = m_write_buf;
        m_iv[0].iov_len = m_write_idx;
        m_iv[1].iov_base = m_real_file_addr;
        m_iv[1].iov_len = m_file_stat.st_size;
        m_iv_num = 2;
        m_bytes_to_send = m_write_idx + m_file_stat.st_size;
        return true;
        break;
    }
    case LOG_REG_RESPONSE:
    {
        add_state_line(200, ok_200_title);
        if(!add_map_content()){
            return false;
        }
        break;
    }
    default:
    {
        LOG_ERROR(m_utils->getLogger()) << "UNKNOW HTTP_CODE";
        return false;
        break;
    }
    }
    m_iv[0].iov_base = m_write_buf;
    m_iv[0].iov_len = m_write_idx;
    m_iv_num = 1;
    m_bytes_to_send = m_write_idx;
    return true;
}

void Http::close_conn()
{
    int sockfd = m_client->getFd();
    if (sockfd != -1)
    {
        cb_func(m_client, m_epollfd);
        sockfd = -1;
    }
}
