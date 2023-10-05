#include "webServer.h"

int webServer::m_client_nums = 0;
std::unordered_map<string, string> webServer::m_users;

void webServer::init_root()
{
    string serverPathStr = m_utils->getCurPath();
    serverPathStr.append("/root");
    m_root = (char *)malloc(serverPathStr.size() + 1);
    strcpy(m_root, serverPathStr.c_str());
}

void webServer::init_util()
{
    m_utils = Utils::getInstance();
    m_utils->setTimeSlot(TIMESLOT);
}

void webServer::init_threadPool()
{
    try
    {
        m_threadPool = new theadPool<Http>;
    }
    catch (const std::exception &e)
    {
        LOG_ERROR(m_utils->getLogger()) << "线程池内存分配失败";
        exit(-1);
    }
}

void webServer::init_http()
{
    try
    {
        m_http = new Http[MAX_FD];
    }
    catch (const std::exception &e)
    {
        LOG_ERROR(m_utils->getLogger()) << "HTTP处理器内存分配失败";
        exit(-1);
    }
}

void webServer::init_config()
{
    m_config = Config::getInstance();
}

void webServer::init_trigMode()
{
    int trigMode = m_config->getTrigMode();
    switch (trigMode)
    {
    case 0: // LT + LT
    {
        m_listen_trigMode = 0;
        m_conn_trigMode = 0;
        break;
    }
    case 1: // LT + ET
    {
        m_listen_trigMode = 0;
        m_conn_trigMode = 1;
        break;
    }
    case 2: // ET + LT
    {
        m_listen_trigMode = 1;
        m_conn_trigMode = 0;
        break;
    }
    case 3: // ET + ET
    {
        m_listen_trigMode = 1;
        m_conn_trigMode = 1;
        break;
    }
    default:
        m_listen_trigMode = 0;
        m_conn_trigMode = 0;
        break;
    }
}

void webServer::init_linger()
{
    m_linger = m_config->getLinger();
    // 优雅关闭连接
    if (0 == m_linger)
    {
        struct linger tmp = {0, 1};
        setsockopt(m_servfd, SOL_SOCKET, SO_LINGER, &tmp, sizeof(tmp));
    }
    else if (1 == m_linger)
    {
        struct linger tmp = {1, 1};
        setsockopt(m_servfd, SOL_SOCKET, SO_LINGER, &tmp, sizeof(tmp));
    }
}

void webServer::init_client()
{
    m_clients = new Client[MAX_FD];
}

void webServer::init_signal()
{
    m_utils->add_sig(SIGPIPE, SIG_IGN);
    m_utils->add_sig(SIGINT, Utils::sig_handler);
    m_utils->add_sig(SIGTSTP, Utils::sig_handler);
    m_utils->add_sig(SIGALRM, Utils::sig_handler);
    alarm(TIMESLOT);
}

void webServer::init_server()
{
    // 端口号
    m_port = m_config->getPort();

    m_servfd = socket(PF_INET, SOCK_STREAM, 0);
    memset(&m_serv_addr, 0, sizeof(m_serv_addr));
    m_serv_addr.sin_family = AF_INET;
    m_serv_addr.sin_addr.s_addr = INADDR_ANY;
    m_serv_addr.sin_port = htons(m_port);

    int reuse = 1;
    setsockopt(m_servfd, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse));

    if (bind(m_servfd, (struct sockaddr *)&m_serv_addr, sizeof(m_serv_addr)) == -1)
    {
        LOG_ERROR(m_utils->getLogger()) << "bind error";
        throw std::exception();
        exit(-1);
    }

    if (listen(m_servfd, 5) == -1)
    {
        LOG_ERROR(m_utils->getLogger()) << "listen error";
        throw std::exception();
        exit(-1);
    }

    if ((m_epollfd = epoll_create(5)) == -1)
    {
        LOG_ERROR(m_utils->getLogger()) << "epoll_create error";
        throw std::exception();
        exit(-1);
    }

    // 将监听的文件描述符添加到epoll对象中
    m_utils->add_fd(m_epollfd, m_servfd, false, m_listen_trigMode);

    // 创建管道
    if (socketpair(PF_UNIX, SOCK_STREAM, 0, m_pipefd) == -1)
    {
        LOG_ERROR(m_utils->getLogger()) << "socketpair error";
        throw std::exception();
    }

    // 设置管道写端为非阻塞，如果写缓冲区满了，立即返回，减少信号处理函数的执行时间
    m_utils->setnoblocking(m_pipefd[1]);
    m_utils->add_fd(m_epollfd, m_pipefd[0], false, 0);

    Utils::u_pipefd = m_pipefd;
    Utils::u_epollfd = m_epollfd;
}

bool webServer::handleClient()
{
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    if (0 == m_listen_trigMode)
    {
        int client_sock = accept(m_servfd, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_sock < 0)
        {
            LOG_FMT_ERROR(m_utils->getLogger(),"epoll_create error %s",strerror(errno));
            return false;
        }
        if (m_client_nums >= MAX_FD)
        {
            LOG_FMT_ERROR(m_utils->getLogger(),"m_client_nums overflow %ds",m_client_nums);
            return false;
        }
        initClient(client_sock, client_addr);
        m_client_nums++;
    }
    else
    {
        while (1)
        {
            int client_sock = accept(m_servfd, (struct sockaddr *)&client_addr, &client_addr_len);
            if (client_sock < 0)
            {
                cout << "accept erro :" << client_sock << endl;
                break;
            }
            if (m_client_nums >= MAX_FD)
            {
                LOG_FMT_ERROR(m_utils->getLogger(),"m_client_nums overflow %ds",m_client_nums);
                break;
            }
            initClient(client_sock, client_addr);
            m_client_nums++;
        }
        return false;
    }
    return true;
}

bool webServer::handleSignal()
{
    int sig = 0;
    char signals[1024];
    int ret = recv(m_pipefd[0], signals, sizeof(signals), 0);
    if (ret == -1 || ret == 0)
    {
        return false;
    }
    else
    {
        for (int i = 0; i < ret; ++i)
        {
            switch (signals[i])
            {
            case SIGINT:
            {
                m_stop = true;
                break;
            }
            case SIGTSTP:
            {
                m_stop = true;
                break;
            }
            case SIGALRM:
            {
                m_timeout = true;
            }
            default:
                break;
            }
        }
    }
    return true;
}

/**
 * @brief 有数据传输，将定时器往后延时
 *
 * @param timer
 */
void webServer::adjust_timer(Client* client,TimerNode *timer)
{
    m_utils->m_timerWheel->del_timer(timer);
    TimerNode* newTimer = m_utils->m_timerWheel->add_timer(calculate_timeout());
    newTimer->cb_func = cb_func;
    newTimer->m_client = client;
    client->setTimer(newTimer);
}

void webServer::deal_timer(TimerNode *timer, int sockfd)
{
    timer->cb_func(&m_clients[sockfd], m_epollfd);
    if (timer)
    {
        m_utils->m_timerWheel->del_timer(timer);
    }
}

void webServer::handleRead(int sockfd)
{
    TimerNode *timer = m_clients[sockfd].getTimer();
    if (m_clients[sockfd].getHttp()->read_from())
    {
        m_threadPool->add(m_clients[sockfd].getHttp());
        if (timer)
        { 
            adjust_timer(&m_clients[sockfd],timer);
        }
    }
    else
    {
        
        deal_timer(timer, sockfd);
    }
}

void webServer::handleWrite(int sockfd)
{
    TimerNode *timer = m_clients[sockfd].getTimer();
    bool ret = m_clients[sockfd].getHttp()->write_to();
    if (ret)
    {
        if (timer)
        {
            adjust_timer(&m_clients[sockfd],timer);
        }
    }
    else
    {
        deal_timer(timer, sockfd);
    }
}

void webServer::removeTimer(TimerNode *timer, int sockfd)
{
    timer->cb_func(&m_clients[sockfd], m_epollfd);
    if (timer)
    {
        m_utils->m_timerWheel->del_timer(timer);
    }
    LOG_FMT_INFO(m_utils->m_logger, "关闭连接 fd = %d", sockfd);
}

void webServer::initClient(int clientfd, struct sockaddr_in client_address)
{
    m_http[clientfd].setClient(&m_clients[clientfd]);
    m_clients[clientfd].init(clientfd, client_address, m_conn_trigMode, m_epollfd,&m_http[clientfd],m_root);
    TimerNode *timer = m_utils->m_timerWheel->add_timer(calculate_timeout());
    timer->cb_func = cb_func;
    timer->m_client = &m_clients[clientfd];
    m_clients[clientfd].setTimer(timer);
}

int webServer::calculate_timeout(){
    time_t cur = time(nullptr);
    //15秒后触发
    time_t expire_time = cur + 3 * TIMEINTERVAL;
    
    //计算时间差
    return expire_time - cur;
}

webServer::webServer() : m_stop(false), m_timeout(false)
{
    init_util();
    init_root();
    init_config();
    init_threadPool();
    init_http();
    init_trigMode();
    init_linger();
    init_signal();
    init_client();
    init_server();
}

void webServer::start()
{
    printf("server is running and port is %d....\n", m_port);
    while (!m_stop)
    {
        int nums = epoll_wait(m_epollfd, m_events, MAX_EVENT_NUM, -1);
        if (nums < 0 && errno != EINTR)
        {
            LOG_ERROR(m_utils->getLogger()) << "epoll_wait failed";
            break;
        }

        for (int i = 0; i < nums; i++)
        {
            int sockfd = m_events[i].data.fd;

            // 客户端连接
            if (sockfd == m_servfd)
            {
                int sockfd = m_events[i].data.fd;
                cout << "new connection is coming" << endl;
                bool ret = handleClient();
                if (!ret)
                    continue;
            }

            // 信号处理
            else if ((m_events[i].events & EPOLLIN) && (sockfd == m_pipefd[0]))
            {
                int ret = handleSignal();
                if (!ret)
                {
                    LOG_INFO(m_utils->getLogger()) << "handleSignal error";
                }
            }

            // 读事件
            else if (m_events[i].events & EPOLLIN)
            {
                handleRead(sockfd);
            }

            // 写事件，一次性写完
            else if (m_events[i].events & EPOLLOUT)
            {
                handleWrite(sockfd);
            }

            // 客户端关闭连接或者异常
            else if (m_events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))
            {
                // 客户端异常断开或者错误事件，清理资源
                TimerNode *timer = m_clients[sockfd].getTimer();
                removeTimer(timer, sockfd);
            }
        }
        // 处理定时器为非必须事件，收到信号并不是立马处理
        // 完成读写事件后，再进行处理
        if (m_timeout)
        {
            m_utils->timer_handler();
            m_timeout = false;
        }
    }
}

webServer::~webServer()
{
    close(m_epollfd);
    close(m_servfd);
    free(m_root);
    delete[] m_clients;
    delete[] m_http;
    delete m_threadPool;
}