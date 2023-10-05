#pragma once
#include <pthread.h>
#include <list>
#include <exception>
#include <cstdio>
#include "../Locker/locker.h"
#include "../Util/utils.h"

template <class T>
class theadPool
{
public:
    theadPool(int thread_nums = 8, int max_req_nums = 10000);
    ~theadPool();
    bool add(T *request); // 添加任务

private:
    static void *worker(void *arg); // 类内要声明为静态
    void run();                     // 从工作队列取任务执行

private:
    // 线程数量
    int m_thread_nums;

    // 线程池数组
    pthread_t *m_threads;

    // 请求队列最多允许的等待处理的请求数量
    int m_max_req_nums;

    // 请求队列
    std::list<T *> m_work_queue;

    // 队列互斥锁
    Locker m_queue_lock;

    // 信号量，判断队列是否有任务处理
    Sem m_queue_stat;

    // 是否结束线程标志位
    bool m_stop;

    Utils* m_utils;
};

template <class T>
theadPool<T>::theadPool(int thread_nums, int max_req_nums)
    : m_thread_nums(thread_nums), m_max_req_nums(max_req_nums),
      m_stop(false), m_threads(NULL),m_utils(Utils::getInstance())
{
    if (thread_nums <= 0 || m_max_req_nums <= 0)
    {
        LOG_ERROR(m_utils->m_logger) << "thread_nums <= 0 || m_max_req_nums <= 0"; 
        throw std::exception();
    }

    m_threads = new pthread_t[m_thread_nums];
    if (!m_threads)
    {
        LOG_ERROR(m_utils->m_logger) << "m_threads new error"; 
        throw std::exception();
    }

    // 创建线程，并将他们设置位线程脱离
    for (int i = 0; i < m_thread_nums; i++)
    {

        if (pthread_create(m_threads + i, NULL, worker, this) != 0)
        {
            LOG_ERROR(m_utils->m_logger) << "pthread_create error";
            delete[] m_threads; 
            throw std::exception();
        }
        if (pthread_detach(m_threads[i]))
        {
            LOG_ERROR(m_utils->m_logger) << "pthread_detach error"; 
            delete[] m_threads;
            throw std::exception();
        }
    }
}

template <class T>
theadPool<T>::~theadPool()
{
    delete[] m_threads;
    m_stop = true;
}

template <class T>
bool theadPool<T>::add(T *request)
{
    m_queue_lock.lock();

    // 请求队列已经满了
    if (m_work_queue.size() > m_max_req_nums)
    {
        LOG_ERROR(m_utils->m_logger) << "请求队列已经满了"; 
        m_queue_lock.unlock();
        return false;
    }

    m_work_queue.push_back(request);
    m_queue_lock.unlock();
    m_queue_stat.post(); // 信号量+1

    return true;
}

template <class T>
void *theadPool<T>::worker(void *arg)
{
    theadPool *pool = (theadPool *)arg;
    pool->run();
    return pool;
}

template <class T>
void theadPool<T>::run()
{
    while (!m_stop)
    {
        m_queue_stat.wait(); // 是否有任务
        m_queue_lock.lock();
        if (m_work_queue.empty())
        {
            m_queue_lock.unlock();
            continue;
        }

        T *request = m_work_queue.front();
        m_work_queue.pop_front();
        m_queue_lock.unlock();

        if (!request)
        {
            continue;
        }
        request->process();
    }
}
