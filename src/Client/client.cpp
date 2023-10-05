#include "client.h"
#include "../HTTP/Http.h"

Client::Client() : m_sockfd(0), m_trigMode(0),timer(nullptr),http(nullptr),m_epollfd(0)
{

}

Client::~Client()
{
}

void Client::init(int fd, sockaddr_in& addr,int trigMode,int epollfd,Http* http,char* root)
{
    m_sockfd = fd;
    m_addr = addr;
    m_trigMode = trigMode;
    m_epollfd = epollfd;
    this->http = http;
    this->http->init(root);
}

void Client::setTimer(TimerNode* timer){
    this->timer = timer;
}
