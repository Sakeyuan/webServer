#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <exception>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <signal.h>
#include <iostream>
#include <string>
#include <unordered_map>
#include "../Locker/locker.h"
#include "../threadPool/threadPool.h"
#include "../HTTP/Http.h"
#include "../Util/utils.h"
#include "../Config/Config.h"
#include "../Timer/timer_wheel.h"

using std::cout;
using std::endl;
using std::string;

const int MAX_FD = 65535;           //  最大客户端数
const int MAX_EVENT_NUM = 10000;    //  最大监听数量
const int TIMESLOT = 1;             //  时间轮指针转动一格的秒数
const int TIMEINTERVAL = 15;        //  3 * TIMEINTERVAL 为定时任务的定时时间

// 信号管道
static int m_pipefd[2];

class webServer
{
public:
    webServer();
    ~webServer();
    void start();

public:
    // 端口号
    int m_port; 

    // 是否开启服务器
    int m_stop; 

    //根目录
    char* m_root;

    // 线程连接池
    theadPool<Http> *m_threadPool;



    struct sockaddr_in m_serv_addr; // 服务器地址

    struct sockaddr_in clnt_addr; // 客户端地址

    int m_servfd; // 客户端文件描述符

    static int m_client_nums;
    
    //用户信息
    static std::unordered_map<string, string> m_users;

    int m_linger;

    int m_epollfd;

    epoll_event m_events[MAX_EVENT_NUM];

    int m_listen_trigMode;

    int m_conn_trigMode;

    bool m_timeout;

    Client *m_clients;
    Http *m_http; 
    Utils *m_utils;
    Config *m_config;

private:
    void init_util();
    void init_threadPool();
    void init_http();
    void init_config();
    void init_trigMode();
    void init_root();
    void init_linger();
    void init_signal();
    void init_client();
    void init_server();

    bool handleClient();
    bool handleSignal();
    void handleRead(int sockfd);
    void handleWrite(int sockfd);
    void removeTimer(TimerNode* timer,int sockfd);
    void initClient(int clientfd, struct sockaddr_in client_address);
    void adjust_timer(Client* client,TimerNode *timer);
    void deal_timer(TimerNode *timer, int sockfd);
    int calculate_timeout();
};
