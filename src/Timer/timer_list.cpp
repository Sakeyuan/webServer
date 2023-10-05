#include "timer_list.h"
#include "../HTTP/Http.h"
//#include "../webServer/webServer.h"

// void cb_func(Client *client, int epollfd)
// {
//     Utils::getInstance()->remove_fd(epollfd, client->getFd());
//     cout << "关闭连接" << client->getFd() << endl;
//     //webServer::m_client_nums--;
// }

TimerNode::TimerNode() : prev(nullptr), next(nullptr)
{
}

TimerNode::~TimerNode()
{
}

TimerList::TimerList() : m_head(nullptr), m_tail(nullptr)
{
}

TimerList::~TimerList()
{
    TimerNode *p;
    while (p)
    {
        m_head = p->next;
        delete p;
        p = m_head;
    }
}

void TimerList::add_timer(TimerNode *timer)
{
    if (!timer)
    {
        return;
    }
    if (!m_head)
    {
        m_head = timer;
        m_tail = timer;
        return;
    }
    // 如果新的定时器超时时间小于当前头部结点
    // 直接将当前定时器结点作为头部结点
    if (timer->expire < m_head->expire)
    {
        timer->next = m_head;
        m_head->prev = timer;
        m_head = timer;
        return;
    }
    add_timer(timer, m_head);
}

void TimerList::adjust_timer(TimerNode *timer)
{
    if (!timer)
    {
        return;
    }
    // 被调整的定时器在链表尾部,不做处理
    TimerNode *nextTimer = timer->next;
    if (!nextTimer || (timer->expire < nextTimer->expire))
    {
        return;
    }

    // 被调整定时器是链表头结点，将定时器取出，重新插入
    if (timer == m_head)
    {
        m_head = m_head->next;
        m_head->prev = nullptr;
        timer->next = nullptr;
        add_timer(timer, m_head);
    }
    // 被调整定时器在内部，将定时器取出，重新插入
    else
    {
        timer->prev->next = timer->next;
        timer->next->prev = timer->prev;
        add_timer(timer, timer->next);
    }
}

void TimerList::del_timer(TimerNode *timer)
{
    if (!timer)
    {
        return;
    }
    if (timer == m_head && timer == m_tail)
    {
        m_head = nullptr;
        m_tail = nullptr;
    }
    else if (timer == m_head && timer != m_tail)
    {
        m_head = m_head->next;
        m_head->prev = nullptr;
    }
    else if (timer == m_tail)
    {
        m_tail = m_tail->prev;
        m_tail->next = nullptr;
    }
    else
    {
        timer->prev->next = timer->next;
        timer->next->prev = timer->prev;
    }
    delete timer;
    timer = nullptr;
}

/**
 * @brief 将目标定时器timer插入到节点listHead之后的节点
 *
 * @param timer
 * @param listHead
 */
void TimerList::add_timer(TimerNode *timer, TimerNode *listHead)
{   
    TimerNode *pre = listHead;
    TimerNode *nextTimer = listHead->next;

    // 遍历当前结点之后的链表，按照超时时间找到目标定时器对应的位置，常规双向链表插入操作
    while (nextTimer)
    {
        // 把timer放置在listHead和listHead->next之间
        if (timer->expire < nextTimer->expire)
        {
            pre->next = timer;
            timer->prev = pre;
            timer->next = nextTimer;
            nextTimer->prev = timer;
            break;
        }
        pre = nextTimer;
        nextTimer = nextTimer->next;
    }

    // 遍历完发现，目标定时器需要放到尾结点处
    if (!nextTimer)
    {
        pre->next = timer;
        timer->prev = pre;
        timer->next = nullptr;
        m_tail = timer;
    }
}

void TimerList::tick()
{
    if (!m_head)
    {
        return;
    }
    time_t cur = time(NULL);

    TimerNode *p = m_head;
    while (p)
    {
        // 当前时间小于定时器的超时时间，后面的定时器也没有到期
        if (cur < p->expire)
        {
            break;
        }
        // 当前定时器到期，则调用回调函数，执行定时事件
        p->cb_func(p->client,Utils::u_epollfd);

        // 将处理后的定时器从链表容器中删除，并重置头结点
        m_head = p->next;
        if (m_head)
        {
            m_head->prev = nullptr;
        }
        delete p;
        p = m_head;
    }
}