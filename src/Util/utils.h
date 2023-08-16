#pragma once
#include <fcntl.h>
#include <iostream>
#include <sys/epoll.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unordered_map>
#include <string>
#include <time.h>
#include "../sqlConnPool/sqlConnPool.h"
#include "../Locker/locker.h"
#include "../Logger/log.h"

class TimerWheel;

class Utils
{
private:
    Utils();
    ~Utils();

public:
    static Utils* getInstance();

    int setnoblocking(int fd);

    void remove_fd(int epollfd, int fd);

    void mod_fd(int epollfd, int fd, int ev, int trigMode);

    void add_fd(int epollfd, int fd, bool one_shot, int TrigMode);

    static void sig_handler(int signal);

    void add_sig(int signal, void (*handler)(int), bool restart = false);

    void timer_handler();

    void setTimeSlot(int timeout);

    void reduceClients();

    int getClients() const;
    
    std::unordered_map<std::string, std::string> &getUsers();

    void init_sqlConnPool();

    void getSqlUser();

    bool sqlInsertNP(string userName,string userPwd);

    void init_log();

    Logger::ptr getLogger() { return m_logger; }

    std::string getCurTime(std::string format = "%Y-%m-%d");

    std::string getCurPath();

public:
    Locker m_locker;

    // 管道
    static int *u_pipefd;

    //epoll句柄
    static int u_epollfd;

    // 时间轮定时器
    TimerWheel* m_timerWheel;

    //间隔时间
    int m_timerSlot;

    //数据库连接池
    sqlConnPool *m_sqlConnPool;

    //日志
    LoggerManager* LogMgr;

    //webServer日志对象
    Logger::ptr m_logger;
};
