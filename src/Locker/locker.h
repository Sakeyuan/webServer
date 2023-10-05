#pragma once
#include<pthread.h>
#include<semaphore.h>
#include<exception>
/**
 * 线程同步机制封装类,利用互斥锁、条件变量、信号量实现
**/

class Locker{
public:
    Locker();       //初始化互斥量
    ~Locker();      //销毁互斥量

    bool lock();    //上锁
    bool unlock();  //解锁
    pthread_mutex_t* get();  //获取互斥量

private:
    pthread_mutex_t m_mutex;
};


/**
 * @brief   
 * 条件变量:
 *    线程间的通知方式，当条件变量满足某个条件是唤醒正在等待的线程
 */

class Cond{
public:
    Cond(/* args */);
    ~Cond();

    bool wait(pthread_mutex_t *mutex);    
    bool timed_wait(pthread_mutex_t *mutex,struct timespec t);
    bool signal();      //将一个或者多个线程唤醒
    bool broadcast();  //将全部线程唤醒

private:
   pthread_cond_t  m_cond;
};

//信号量类
class Sem{
public:
    Sem();
    Sem(int num);
    ~Sem();
    bool wait();  //等待信号量
    bool post();  //增加信号量  

private:
    sem_t m_sem;
};
