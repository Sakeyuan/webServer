#pragma once
#include <arpa/inet.h>

class TimerList;
class TimerNode;
class Http;

class Client
{
public:
    Client();
    ~Client();
    void init(int fd, sockaddr_in& addr,int trigMode,int epollfd,Http* http,char* root);
    int getFd() { return m_sockfd; }
    int getEpollfd() const { return m_epollfd; }
    int getTrigMode() const { return m_trigMode; }
    sockaddr_in &getAddr() { return m_addr; }
    TimerNode *getTimer() { return timer; };
    Http *getHttp() { return http; }
    void setTimer(TimerNode *timer);

private:
    //文件描述符
    int m_sockfd;

    //客户端地址
    sockaddr_in m_addr;

    //客户端触发模式
    int m_trigMode;

    //定时器
    TimerNode *timer;

    //Http报文解析器
    Http *http;

    //epoll文件描述符
    int m_epollfd;
};
