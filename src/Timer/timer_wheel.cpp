#include "timer_wheel.h"

// 时间轮槽的数目
const int N = 60;

// 每1s时间轮转动一次，即槽间隔为1s
const int SI = 1;

void cb_func(Client *client, int epollfd)
{
    Utils* utils = Utils::getInstance();
    utils->remove_fd(epollfd, client->getFd());
    //printf("连接关闭 %d",client->getFd());
    //LOG_FMT_INFO(utils->getLogger(), "连接关闭 %d", client->getFd());
    utils->reduceClients();
}

TimerNode::TimerNode(int rot, int ts)
    : next(nullptr), prev(nullptr), rotation(rot), timer_slot(ts)
{
}
TimerNode::~TimerNode()
{
}

TimerWheel::TimerWheel() : curSlot(0)
{
    for (int i = 0; i < N; i++)
    {
        slots[i] = nullptr;
    }
}

TimerWheel::~TimerWheel()
{
    // 遍历每个槽，并销毁其中的定时器
    for (int i = 0; i < N; i++)
    {
        TimerNode *p = slots[i];
        while (p)
        {
            slots[i] = p->next;
            delete p;
            p = slots[i];
        }
    }
}

// 根据timeout创建一个定时器，并把他插入合适的槽中
TimerNode *TimerWheel::add_timer(int timeout)
{
    if (timeout < 0)
    {
        return nullptr;
    }
    int ticks = 0;
    ticks = timeout < SI ? 1 : timeout / SI;
    // 计算待插入的定时器在时间轮转动多少圈后被触发
    int rotation = ticks / (N * SI);

    // 计算待插入的定时器被插入哪个槽中
    int ts = (curSlot + (ticks / SI)) % N;

    // 创建新的定时器，它在时间轮转动rotation圈之后被触发，且位于ts槽中
    TimerNode *timer = new TimerNode(rotation, ts);

    // 如果ts槽中没有任何定时器，则把新建的定时器插入其中，并将定时器设置为槽的头节点
    if (!slots[ts])
    {
        //printf("add timer ,rotation is %d ,ts is %d curSlot is %d\n", rotation, ts, curSlot);
        slots[ts] = timer;
    }
    else
    {
        timer->next = slots[ts];
        slots[ts]->prev = timer;
        slots[ts] = timer;
    }
    return timer;
}

// 删除目标定时器
void TimerWheel::del_timer(TimerNode *timer)
{
    if (!timer)
    {
        return;
    }
    int ts = timer->timer_slot;
    if (timer == slots[ts])
    {
        slots[ts] = slots[ts]->next;
        if (slots[ts])
        {
            slots[ts]->prev = nullptr;
        }
        delete timer;
    }
    else
    {
        timer->prev->next = timer->next;
        if (timer->next)
        {
            timer->next->prev = timer->prev;
        }
        delete timer;
    }
}

void TimerWheel::tick()
{
    // 取得时间轮上当前槽的头节点
    TimerNode *p = slots[curSlot];
    //printf("current slot is %d\n", curSlot);
    while (p)
    {
        // 如果定时器rotation值大于0，则它在这一轮不起作用
        if (p->rotation > 0)
        {
            p->rotation--;
            p = p->next;
        }
        // 否则，说明定时器到期了，执行定时任务，然后删除定时器
        else
        {
            p->cb_func(p->m_client, Utils::u_epollfd);
            if (p == slots[curSlot])
            {
                slots[curSlot] = p->next;
                delete p;
                if (slots[curSlot])
                {
                    slots[curSlot]->prev = nullptr;
                }
                p = slots[curSlot];
            }
            else
            {
                p->prev->next = p->next;
                if (p->next)
                {
                    p->next->prev = p->prev;
                }
                TimerNode *q = p->next;
                delete p;
                p = q;
            }
        }
    }

    // 更新时间轮的当前槽，反映时间轮的转动
    curSlot = ++curSlot % N;
}

