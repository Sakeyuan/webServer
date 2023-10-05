#include "log.h"

Locker LoggerManager::m_locker;

class MessageFormatItem : public LogFormatter::FormatItem
{
public:
    MessageFormatItem(const std::string &str = "") {}
    void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
    {
        os << event->getContent();
    }
};

class LevelFormatItem : public LogFormatter::FormatItem
{
public:
    LevelFormatItem(const std::string &str = "") {}
    void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
    {
        os << LogLevel::toString(level);
    }
};

class ElapseFormatItem : public LogFormatter::FormatItem
{
public:
    ElapseFormatItem(const std::string &str = "") {}
    void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
    {
        os << event->getElapse();
    }
};

class NameFormatItem : public LogFormatter::FormatItem
{
public:
    NameFormatItem(const std::string &str = "") {}
    void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
    {
        os << logger->getName();
    }
};

class ThreadIdFormatItem : public LogFormatter::FormatItem
{
public:
    ThreadIdFormatItem(const std::string &str = "") {}
    void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
    {
        os << event->getThreadId();
    }
};

class FiberIdFormatItem : public LogFormatter::FormatItem
{
public:
    FiberIdFormatItem(const std::string &str = "") {}
    void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
    {
        os << event->getFiberId();
    }
};

class DateTimeFormatItem : public LogFormatter::FormatItem
{
public:
    DateTimeFormatItem(const std::string &format = "%Y-%m-%d %H-%M-%S") : m_format(format)
    {
        if (m_format.empty())
        {
            m_format = "%Y-%m-%d %H:%M%S";
        }
    }

    void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
    {
        struct tm tm;
        time_t time = event->getTime();
        localtime_r(&time, &tm);
        char buf[64];
        strftime(buf, sizeof(buf), m_format.c_str(), &tm);
        os << buf;
    }

private:
    std::string m_format;
};

class FileNameFormatItem : public LogFormatter::FormatItem
{
public:
    FileNameFormatItem(const std::string &str = "") {}
    void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
    {
        os << event->getFile();
    }
};

class LineFormatItem : public LogFormatter::FormatItem
{
public:
    LineFormatItem(const std::string &str = "") {}
    void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
    {
        os << event->getLine();
    }
};

class NewLineFormatItem : public LogFormatter::FormatItem
{
public:
    NewLineFormatItem(const std::string &str = "") {}
    void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
    {
        os << std::endl;
    }
};

class StringFormatItem : public LogFormatter::FormatItem
{
public:
    StringFormatItem(const std::string &str) : m_string(str)
    {
    }
    void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
    {
        os << m_string;
    }

private:
    std::string m_string;
};

class TabFormatItem : public LogFormatter::FormatItem
{
public:
    TabFormatItem(const std::string &str = "")
    {
    }
    void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
    {
        os << "\t";
    }

private:
    std::string m_string;
};

class FiberFormatItem : public LogFormatter::FormatItem
{
public:
    FiberFormatItem(const std::string &str = "") {}
    void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
    {
        os << event->getFiberId();
    }
};

Logger::Logger(const std::string name) : m_name(name), m_level(LogLevel::DEBUG)
{
    m_formatter.reset(new LogFormatter("%d{%Y-%m-%d %H-%M-%S}%T[%p]%T[%c]%T%t%T%F%T%f:%l%T%m%n"));
}

LogEvent::LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level, const char *file, int32_t line, uint64_t elapse, uint32_t thread_id, uint32_t fiber_id, uint64_t time)
    : m_file(file), m_line(line), m_elapse(elapse), m_threadId(thread_id), m_fiberId(fiber_id), m_time(time), m_logger(logger), m_level(level)
{
}

const char *LogLevel::toString(LogLevel::Level level)
{
    switch (level)
    {
#define XX(name)         \
    case LogLevel::name: \
        return #name;    \
        break;
        XX(DEBUG);
        XX(INFO);
        XX(WARN);
        XX(ERROR);
        XX(FATAL);
#undef XX

    default:
        return "UNKNOW";
    }
    return "UNKNOW";
}

void Logger::log(LogLevel::Level level, LogEvent::ptr event)
{
    if (level >= m_level)
    {
        // 获取自身类的智能指针
        auto self = shared_from_this();
        if (!m_appenders.empty())
        {
            for (auto &it : m_appenders)
            {
                it->log(self, level, event);
            }
        }
        else if(m_root) {
            m_root->log(level, event);
        }
    }
}

void Logger::debug(LogEvent::ptr event)
{
    log(LogLevel::DEBUG, event);
}

void Logger::info(LogEvent::ptr event)
{
    log(LogLevel::INFO, event);
}

void Logger::warn(LogEvent::ptr event)
{
    log(LogLevel::WARN, event);
}

void Logger::error(LogEvent::ptr event)
{
    log(LogLevel::ERROR, event);
}

void Logger::fatal(LogEvent::ptr event)
{
    log(LogLevel::FATAL, event);
}

void Logger::addAppender(LogAppender::ptr appender)
{
    LoggerManager::m_locker.lock();
    if (!appender->getFormatter())
    {
        appender->setFormater(m_formatter);
    }
    m_appenders.push_back(appender);
    LoggerManager::m_locker.unlock();
}

void Logger::delAppender(LogAppender::ptr appender)
{
    LoggerManager::m_locker.lock();
    for (auto it = m_appenders.begin(); it != m_appenders.end(); ++it)
    {
        if (*it == appender)
        {
            m_appenders.erase(it);
            break;
        }
    }
    LoggerManager::m_locker.unlock();
}

void Logger::clearAppenders(){
    LoggerManager::m_locker.lock();
    m_appenders.clear();
    LoggerManager::m_locker.unlock();
}

FileLogAppender::FileLogAppender(const std::string &filename) : m_filename(filename)
{
    reopen();
}

bool FileLogAppender::reopen()
{
    LoggerManager::m_locker.lock();
    if (m_filestream)
    {
        m_filestream.close();
    }
    m_filestream.open(m_filename, std::ios::app);
    LoggerManager::m_locker.unlock();

    return m_filestream.is_open();
}

void FileLogAppender::log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event)
{
    if (level >= m_level)
    {
        LoggerManager::m_locker.lock();
        m_filestream << m_formatter->format(logger, level, event);
        LoggerManager::m_locker.unlock();
    }
}

void StdoutLogAppender::log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event)
{
    if (level >= m_level)
    {
        LoggerManager::m_locker.lock();
        std::cout << m_formatter->format(logger, level, event);
        LoggerManager::m_locker.unlock();
    }
}

LogFormatter::LogFormatter(const std::string &pattern) : m_pattern(pattern)
{
    init();
}

std::string LogFormatter::format(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event)
{
    std::stringstream ss;
    for (auto &it : m_items)
    {
        it->format(ss, logger, level, event);
    }
    return ss.str();
}

//%X %X{} %X[X]
/**
 * %m -- 消息体
 * %p -- level
 * %r -- 启动后的时间
 * %c -- 日志名称
 * %t -- 线程Id
 * %n -- 回车
 * %d -- 时间
 * %f -- 文件名
 * %l -- 行号
 * %T -- Tab
 * */

void LogFormatter::init()
{
    // str format type
    // type == 0 特殊符号(例如[ ] :字符) type == 1 m、p、r、c等标识符号
    std::vector<std::tuple<std::string, std::string, int>> vec;
    std::string nstr;   //存储特殊符号(例如[ ] :字符)
    // m_pattern = "%d{%Y-%m-%d %H-%M-%S}%T[%p]%T[%c]%T%t%T%F%T%f:%l%T%m%n"
    for (size_t i = 0; i < m_pattern.size(); ++i)
    {
        // 遇到[ 或者 ] 或者:
        if (m_pattern[i] != '%')
        {
            nstr.append(1, m_pattern[i]);
            continue;
        }
        //%%
        if ((i + 1) < m_pattern.size())
        {
            if (m_pattern[i + 1] == '%')
            {
                nstr.append(1, m_pattern[i]);
                continue;
            }
        }
        size_t n = i + 1;
        // fmt_status == 0 解析str(%d)，fmt_status == 1 解析fmt(%Y-%m-%d %H-%M-%S)
        int fmt_status = 0;
        size_t fmt_begin = 0;

        std::string str;
        std::string fmt;
        while (n < m_pattern.size())
        {
            // 状态0，也不是字母，也不是{ 和 },即m_pattern[n]是[ 或者 ] 或者 % 截取类似%t，或者是:
            if (!fmt_status && (!isalpha(m_pattern[n]) && m_pattern[n] != '{' && m_pattern[n] != '}'))
            {
                str = m_pattern.substr(i + 1, n - i - 1);
                break;
            }
            if (fmt_status == 0)
            {
                if (m_pattern[n] == '{')
                {
                    str = m_pattern.substr(i + 1, n - i - 1);
                    fmt_status = 1; // 解析格式
                    fmt_begin = n;
                    ++n;
                    continue;
                }
            }
            else if (fmt_status == 1)
            {
                if (m_pattern[n] == '}')
                {
                    // 截取 {}里面内容，%Y-%m-%d %H-%M-%S
                    fmt = m_pattern.substr(fmt_begin + 1, n - fmt_begin - 1);
                    fmt_status = 0;
                    ++n;
                    break;
                }
            }
            ++n;
            if (n == m_pattern.size())
            {
                if (str.empty())
                {
                    str = m_pattern.substr(i + 1);
                }
            }
        }

        if (fmt_status == 0)
        {
            if (!nstr.empty())
            {
                //"[" 或者是 "]"
                vec.push_back(std::make_tuple(nstr, std::string(), 0));
                nstr.clear();
            }
            vec.push_back(std::make_tuple(str, fmt, 1));
            i = n - 1;
        }
        else if (fmt_status == 1)
        {
            std::cout << "pattern parse error: " << m_pattern << " - " << m_pattern.substr(i) << std::endl;
            // m_error = true;
            vec.push_back(std::make_tuple("<<pattern_error>>", fmt, 0));
        }
    }
    if (!nstr.empty())
    {
        vec.push_back(std::make_tuple(nstr, "", 0));
    }
    // 映射，不同标识符->不同FormatItem
    static std::map<std::string, std::function<FormatItem::ptr(const std::string &str)>> s_format_items = {
#define XX(str, C)                                                               \
    {                                                                            \
        #str, [](const std::string &fmt) { return FormatItem::ptr(new C(fmt)); } \
    }

        //"m", [](const std::string &fmt) { return FormatItem::ptr(new MessageFormatItem(fmt)); } 
        XX(m, MessageFormatItem),
        XX(p, LevelFormatItem),
        XX(r, ElapseFormatItem),
        XX(c, NameFormatItem),
        XX(t, ThreadIdFormatItem),
        XX(n, NewLineFormatItem),
        XX(d, DateTimeFormatItem),
        XX(f, FileNameFormatItem),
        XX(l, LineFormatItem),
        XX(T, TabFormatItem),
        XX(F, FiberFormatItem)

#undef XX
    };

    for (auto &i : vec)
    {
        // type
        if (std::get<2>(i) == 0)
        {
            m_items.push_back(FormatItem::ptr(new StringFormatItem(std::get<0>(i))));
        }
        else
        {
            auto it = s_format_items.find(std::get<0>(i));
            if (it == s_format_items.end())
            {
                m_items.push_back(FormatItem::ptr(new StringFormatItem("<<error_format %" + std::get<0>(i) + ">>")));
                // m_error = true;
            }
            else
            {
                // fmt
                m_items.push_back(it->second(std::get<1>(i)));
            }
        }
        // std::cout << "(" << std::get<0>(i) << ") - (" << std::get<1>(i) << ") - (" << std::get<2>(i) << ")" << std::endl;
    }
}

LogEventWrap::LogEventWrap(LogEvent::ptr e) : m_event(e)
{
}

LogEventWrap::~LogEventWrap()
{
    m_event->getLogger()->log(m_event->getLevel(), m_event);
}

std::stringstream &LogEventWrap::getSS()
{
    return m_event->getSS();
}

void LogEvent::format(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    format(fmt, args);
    va_end(args);
}

void LogEvent::format(const char *fmt, va_list args)
{
    char *buf = nullptr;
    int len = vasprintf(&buf, fmt, args);
    if (len != -1)
    {
        m_ss << std::string(buf, len);
        free(buf);
    }
}

LoggerManager::LoggerManager()
{
    m_root.reset(new Logger);
    m_root->addAppender(LogAppender::ptr(new StdoutLogAppender));
    //m_locker.lock();
    m_loggers[m_root->m_name] = m_root;
    //m_locker.unlock();
    init();
}

Logger::ptr LoggerManager::getLogger(const std::string &name)
{
    //LoggerManager::m_locker.lock();
    auto it = m_loggers.find(name);
    if (it != m_loggers.end())
    {
        //LoggerManager::m_locker.unlock();
        return it->second;
    }
    Logger::ptr logger(new Logger(name));
    if(logger == nullptr){
        //LoggerManager::m_locker.unlock();
        return logger;
    }
    logger->m_root = m_root;
    m_loggers[name] = logger;
    //LoggerManager::m_locker.unlock();
    return logger;
}

void LoggerManager::init()
{
}

LoggerManager *LoggerManager::getInstance(){
    static LoggerManager instance;
    return &instance;
}