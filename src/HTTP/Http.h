#pragma once
#include <sys/epoll.h>
#include <iostream>
#include <thread>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <error.h>
#include <sys/uio.h>
#include <string.h>
#include <curl/curl.h>
#include <mysql/mysql.h>
#include <unordered_map>
#include "../Locker/locker.h"
#include "../Util/utils.h"
#include "../Client/client.h"
#include "../Timer/timer_wheel.h"
#include "parse_status.h"


using std::cout;
using std::endl;
using std::string;
using namespace Sake;

const int FILENAME_LEN = 200;
const int READ_BUF_SIZE = 2048;
const int WRITE_BUF_SIZE = 2048;

class Http
{
public:
    Http();
    ~Http();

    void init(char *root);

    bool read_from();

    bool write_to();

    void close_conn();

    void process();

    void setClient(Client* client);

private:
    void init();

    HTTP_CODE process_read();

    HTTP_CODE parse_req_line(char *req_text);

    HTTP_CODE parse_req_header(char *req_text);

    HTTP_CODE parse_req_content(char *req_text);

    LINE_STATUS parse_line();

    char *get_line() { return m_read_buf + m_start_line; }

    HTTP_CODE do_request();

    bool process_write(HTTP_CODE ret);
    bool add_state_line(int state, const char *title);
    bool add_response(const char *format, ...);
    bool add_headers(int content_len);
    bool add_content(const char *content);
    bool add_map_content();
    bool add_content_type();
    bool add_content_length(int content_length);
    bool add_iskeep();
    bool add_blank_line();
    void get_resources_path();

    void unmap();

private:
    //epoll文件描述符
    int m_epollfd;

    // http报文缓冲区
    char m_read_buf[READ_BUF_SIZE];

    // 报文缓冲区读取指针
    int m_read_idx;

    // 响应报文头文件缓冲区
    char m_write_buf[WRITE_BUF_SIZE];

    // 报文缓冲区写指针
    int m_write_idx;

    // 分散写入结构体数组
    struct iovec m_iv[2];

    // 分散写入结构体数组个数
    int m_iv_num;

    // 主状态机的状态
    CHECK_STATE m_check_state;

    // 主状态机指针
    int m_check_idx;

    // 每行起始位置
    int m_start_line;

    //是否是post请求
    bool isPost;

    char *m_data;

    // 请求url
    char *m_url;

    // http协议版本
    char *m_version;

    // 请求方法
    METHOD m_method;

    // 请求主机
    char *m_host;

    // 是否保持连接
    bool m_keep;

    //是否登录或者注册成功
    bool m_ok;

    // 请求体长度
    int m_content_length;

    // 要发送的字节数
    int m_bytes_to_send;

    // 已经发送字节数
    int m_bytes_have_send;

    // 资源根目录
    char *doc_path;

    // 和资源根目录拼接构成真正的请求文件路径
    char m_real_file[FILENAME_LEN];

    // 请求文件地址
    char *m_real_file_addr;

    // 文件状态信息
    struct stat m_file_stat;

    std::unordered_map<string, string> resonse_msg;

    Locker m_locker;

    Utils *m_utils;

    Client *m_client;
};
