#pragma once
#include <string>
#include <stdint.h>
#include <memory>
#include <list>
#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <functional>
#include <tuple>
#include <time.h>
#include <string.h>
#include <map>
#include <thread>
#include <pthread.h>
#include <stdarg.h>
#include "../Locker/locker.h"

class Logger;
class LogEvent;
class LogAppender;
class LogFormatter;
class FormatItem;
class StdoutLogAppender;
class FileLogAppender;
class LogLevel;

#define LOG_LEVEL(logger,level) \
    if(logger->getLevel() <= level) \
        LogEventWrap(LogEvent::ptr(new LogEvent(logger,level,__FILE__,__LINE__,0,(uint32_t)pthread_self(),0,time(0)))).getSS()

#define LOG_DEBUG(logger) LOG_LEVEL(logger,LogLevel::Level::DEBUG)
#define LOG_INFO(logger) LOG_LEVEL(logger,LogLevel::Level::INFO)
#define LOG_WARN(logger) LOG_LEVEL(logger,LogLevel::Level::WARN)
#define LOG_ERROR(logger) LOG_LEVEL(logger,LogLevel::Level::ERROR)
#define LOG_FATAL(logger) LOG_LEVEL(logger,LogLevel::Level::FATAL)

#define LOG_FMT_LEVEL(logger,level,fmt,...) \
    if(logger->getLevel() <= level) \
        LogEventWrap(LogEvent::ptr(new LogEvent(logger,level,__FILE__,__LINE__,0,\
        (uint32_t)pthread_self(),0,time(0)))).getEvent()->format(fmt,__VA_ARGS__)

#define LOG_FMT_DEBUG(logger,fmt,...) LOG_FMT_LEVEL(logger,LogLevel::Level::DEBUG,fmt,__VA_ARGS__)
#define LOG_FMT_INFO(logger,fmt,...) LOG_FMT_LEVEL(logger,LogLevel::Level::INFO,fmt,__VA_ARGS__)
#define LOG_FMT_WARN(logger,fmt,...) LOG_FMT_LEVEL(logger,LogLevel::Level::WARN,fmt,__VA_ARGS__)
#define LOG_FMT_ERROR(logger,fmt,...) LOG_FMT_LEVEL(logger,LogLevel::Level::ERROR,fmt,__VA_ARGS__)
#define LOG_FMT_FATAL(logger,fmt,...) LOG_FMT_LEVEL(logger,LogLevel::Level::FATAL,fmt,__VA_ARGS__)

#define LOG_ROOT() LoggerMgr::GetInstance()->getRoot()

//日志级别
class LogLevel {
public:
    enum Level {
        UNKOWN = 0,
        DEBUG = 1,
        INFO = 2,
        WARN = 3,
        ERROR = 4,
        FATAL = 5
    };
    //将日志级别转换成为文本
    static const char* toString(LogLevel::Level level);
};

//日志事件
class LogEvent {
public:
    typedef std::shared_ptr<LogEvent>ptr;
    LogEvent(std::shared_ptr<Logger>logger, LogLevel::Level level
                , const char* file, int32_t line
                , uint64_t elapse, uint32_t thread_id
                , uint32_t fiber_id, uint64_t time);

    const char* getFile() const { return m_file; }
    int32_t getLine() const { return m_line; }
    uint32_t getThreadId() const { return m_threadId; }
    uint32_t getFiberId() const { return m_fiberId; }
    uint64_t getElapse() const { return m_elapse; }
    uint64_t getTime() const { return m_time; }
    std::string getContent() const { return m_ss.str(); }
    std::stringstream& getSS() { return m_ss; }

    std::shared_ptr<Logger>getLogger() { return m_logger; }
    LogLevel::Level getLevel() { return m_level; }

    void format(const char* fmt, ...);
    void format(const char* fmt, va_list al);

private:
    //文件名称
    const char* m_file = nullptr;

    //行号
    int32_t m_line = 0;

    //程序启动到现在的毫秒数
    uint64_t m_elapse = 0;

    //线程Id
    uint32_t m_threadId = 0;

    //协程Id
    uint32_t m_fiberId = 0;

    //时间
    uint64_t m_time = 0;

    //消息
    std::stringstream m_ss;

    //日志器
    std::shared_ptr<Logger> m_logger;
    
    //日志级别
    LogLevel::Level m_level;
};

//logevent日志事件包装器
class LogEventWrap {
public:
    LogEventWrap(LogEvent::ptr e);
    ~LogEventWrap();
    std::stringstream& getSS();
    LogEvent::ptr getEvent() const { return m_event; }

private:
    LogEvent::ptr m_event;
};

//日志格式器
class LogFormatter {
public:
    typedef std::shared_ptr<LogFormatter>ptr;
    std::string format(std::shared_ptr<Logger>logger, LogLevel::Level level, LogEvent::ptr event);
    LogFormatter(const std::string& pattern);

public:
    //日志解析模块
    class FormatItem {
    public:
        typedef std::shared_ptr<FormatItem>ptr;
        virtual ~FormatItem() {}
        virtual void format(std::ostream& os, std::shared_ptr<Logger>logger, LogLevel::Level level, LogEvent::ptr event) = 0;
    };

    //解析pattern
    void init();

private:
    //日志格式模板
    std::string m_pattern;

    //日志解析后的格式
    std::vector<FormatItem::ptr>m_items;
};

//日志输出地
class LogAppender {
    friend class Logger;
    
public:
    typedef std::shared_ptr<LogAppender>ptr;
    virtual ~LogAppender() {}
    virtual void log(std::shared_ptr<Logger>logger, LogLevel::Level level, LogEvent::ptr event) = 0;
    void setFormater(LogFormatter::ptr formatter) { m_formatter = formatter; }
    LogFormatter::ptr getFormatter() const { return m_formatter; }
    LogLevel::Level getLevel() const { return m_level; }
    void setLevel(LogLevel::Level level) { m_level = level; }

protected:
    LogLevel::Level m_level = LogLevel::DEBUG;

    LogFormatter::ptr m_formatter;
};


//日志器
class Logger : public std::enable_shared_from_this<Logger> {
    friend class LoggerManager;
public:
    
    typedef std::shared_ptr<Logger>ptr;
    Logger(const std::string name = "root");

    void log(LogLevel::Level level, LogEvent::ptr event);

    void debug(LogEvent::ptr event);
    void info(LogEvent::ptr event);
    void warn(LogEvent::ptr event);
    void error(LogEvent::ptr event);
    void fatal(LogEvent::ptr event);

    //添加日志落输出地
    void addAppender(LogAppender::ptr appender);
    
    //删除日志输出地
    void delAppender(LogAppender::ptr appender);


    //清空日志输出地
    void clearAppenders();

    LogLevel::Level getLevel() const { return m_level; }
    void setLevel(LogLevel::Level level) { m_level = level; }
    const std::string& getName() const { return m_name; }

private:
    //日志器名称
    std::string m_name;
    
    //日志级别
    LogLevel::Level m_level;

    //输出队列
    std::list<LogAppender::ptr> m_appenders;
    
    //日志格式器
    LogFormatter::ptr m_formatter;

    //主日志器
    Logger::ptr m_root;
};

//输出到控制台
class StdoutLogAppender : public LogAppender {
public:
    typedef std::shared_ptr<StdoutLogAppender>ptr;
    virtual void log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override;
};

//输出到文件
class FileLogAppender : public LogAppender {
public:
    typedef std::shared_ptr<FileLogAppender>ptr;
    FileLogAppender(const std::string& filename);
    virtual void log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override;
    bool reopen();

private:
    std::string m_filename;
    std::ofstream m_filestream;
};

class LoggerManager {
private:
    LoggerManager();
public:
    static LoggerManager *getInstance();
    Logger::ptr getLogger(const std::string &name);
    void init();
    Logger::ptr getRoot() const { return m_root; }

private:
    //日志容器
    std::map<std::string, Logger::ptr>m_loggers;

    //主日志器
    Logger::ptr m_root;

public:
    static Locker m_locker;
};

