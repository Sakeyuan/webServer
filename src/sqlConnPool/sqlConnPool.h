#pragma once
#include <mysql/mysql.h>
#include <stdio.h>
#include <list>
#include <string>
#include "../Locker/locker.h"
#include "../Config/Config.h"

using std::string;

class sqlConnPool
{
private:
    sqlConnPool();
    ~sqlConnPool();
    
public:
    static sqlConnPool *getInstance();
    MYSQL *getConn();
    bool releaseConn(MYSQL* conn);
    int getFreeConnNums();
    void init();

private:
    //最大连接数
    int m_maxConn;

    //当前已使用的连接数
    int m_curConn;

    //当前空闲连接
    int m_freeConn;

    //锁对象
    Locker m_lock;

    //信号量
    Sem m_sem;

    // sql连接池
    std::list<MYSQL *>sqlConnList;

    //配置中心
    Config* m_config;
};


class SqlWrapper{
public:
    SqlWrapper(MYSQL **conn, sqlConnPool *pool);
    ~SqlWrapper();

private:
    MYSQL *m_conn;
    sqlConnPool *m_pool;
};
