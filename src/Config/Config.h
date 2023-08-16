#pragma once
#include <string>
#include <fstream>
#include <string.h>
#include <unistd.h>
using std::fstream;
using std::string;

class Config
{
private:
    Config();
    ~Config();

public:
    void loadConfig();
    static Config *getInstance();

    unsigned int getPort() const { return PORT; }
    int getThreadNums() const { return THREAD_NUMS; }
    int getMAXThreadNums() const { return MAX_TASK_NUMS; }
    int getSqlMaxConn() const { return SQL_MAX_CONN; }
    int getSqlPort() const { return SQL_PORT; }
    string getSqlHost() const { return SQL_HOST; }
    string getSqlUserName() const { return SQL_USER_NAME; }
    string getSqlPassword() const { return SQL_PASSWORD; }
    string getSqlDatabaseName() const { return SQL_DATABASE_NAME; }
    int getTrigMode() const { return TRIGMODE; }
    int getLinger() const { return LINGER; }

private:
    // 端口号
    unsigned int PORT;

    // 线程池的线程数量
    int THREAD_NUMS;

    // 任务队列最大容量
    int MAX_TASK_NUMS;

    // 数据库连接池最大连接
    int SQL_MAX_CONN;

    // 数据库端口
    int SQL_PORT;

    // 数据库主机
    string SQL_HOST;

    // 数据库用户名
    string SQL_USER_NAME;

    // 数据库登录密码
    string SQL_PASSWORD;

    // 使用的数据库名
    string SQL_DATABASE_NAME;

    int TRIGMODE;

    // 是否延迟关闭连接
    int LINGER;

private:
    void removeBlank(string &s);
};
