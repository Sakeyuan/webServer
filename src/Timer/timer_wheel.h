#pragma once
#include <stdio.h>
#include "../Client/client.h"
#include "../Util/utils.h"

void cb_func(Client *client,int epollfd);

class TimerNode{
public:
    TimerNode(int rot, int ts);
    ~TimerNode();

public:
    //记录定时器在时间轮转多少圈后生效
    int rotation;

    //记录定时器属于时间轮上哪个槽
    int timer_slot;

    //回调函数
     void (*cb_func)(Client *,int );

    Client *m_client;

public:
    TimerNode *next;
    TimerNode *prev;
};

class TimerWheel
{
public:
    TimerWheel();
    ~TimerWheel();
    TimerNode *add_timer(int timeout);
    void del_timer(TimerNode *timer);
    void tick();

private:
    //时间轮槽的数目
    static const int N = 60;
    
    //每1s时间轮转动一次，即槽间隔为1s
    static const int SI = 1;

    //时间槽
    TimerNode *slots[N];

    //当前槽
    int curSlot;
};
