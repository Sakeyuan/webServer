#pragma once
#include <time.h>
#include <sys/epoll.h>
#include "../Util/utils.h"
#include "../Client/client.h"

// void cb_func(Client *client,int epollfd);

class TimerNode
{
public:
    TimerNode();
    ~TimerNode();

public:
    //超时时间
    time_t expire;

    //回调函数
    void (*cb_func)(Client *client,int epollfd);
    
    //连接资源
    Client *client;

    //前向定时器
    TimerNode *prev;

    //后向定时器
    TimerNode *next;
};

class TimerList
{
public:
    TimerList();
    ~TimerList();

public:
    void add_timer(TimerNode* timer);
    void adjust_timer(TimerNode* timer);
    void del_timer(TimerNode* timer);
    void tick();

private:
    void add_timer(TimerNode* timer,TimerNode* listHead);
    
    TimerNode *m_head;
    TimerNode *m_tail;
};

