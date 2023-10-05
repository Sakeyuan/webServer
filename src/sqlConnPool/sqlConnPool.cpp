#include "sqlConnPool.h"

sqlConnPool::sqlConnPool()
    : m_maxConn(0), m_curConn(0), m_freeConn(0)
{
    m_config = Config::getInstance();
    this->init();
}

sqlConnPool::~sqlConnPool()
{
    m_lock.lock();
    if (sqlConnList.size() > 0)
    {
        for (auto it = sqlConnList.begin(); it != sqlConnList.end(); ++it)
        {
            MYSQL *conn = *it;
            mysql_close(conn);
        }
        m_curConn = 0;
        m_freeConn = 0;
        sqlConnList.clear();
    }
    m_lock.unlock();
}

MYSQL *sqlConnPool::getConn()
{
    if (0 == sqlConnList.size())
    {
        return nullptr;
    }
    m_sem.wait();
    m_lock.lock();
    MYSQL *conn = sqlConnList.front();
    sqlConnList.pop_front();
    ++m_curConn;
    --m_freeConn;
    m_lock.unlock();
    return conn;
}

bool sqlConnPool::releaseConn(MYSQL *conn)
{
    if (nullptr == conn)
    {
        return false;
    }

    m_lock.lock();
    sqlConnList.push_back(conn);
    ++m_freeConn;
    --m_curConn;
    m_lock.unlock();
    m_sem.post();

    return true;
}

int sqlConnPool::getFreeConnNums()
{
    return this->m_freeConn;
}

void sqlConnPool::init()
{
    m_maxConn = m_config->getSqlMaxConn();
    for (int i = 0; i < m_maxConn; i++)
    {
        MYSQL *conn = nullptr;
        conn = mysql_init(conn);
        if(conn == nullptr){
            printf("mysql_init error \n");
            exit(-1);
        }
        conn = mysql_real_connect(conn,m_config->getSqlHost().c_str(),m_config->getSqlUserName().c_str(),
            m_config->getSqlPassword().c_str(),m_config->getSqlDatabaseName().c_str(),m_config->getSqlPort(),
            nullptr,0);

        if(nullptr == conn){
            exit(-1);
        }
        sqlConnList.push_back(conn);
        ++m_freeConn;
        m_sem = Sem(m_freeConn);
    }
}

sqlConnPool *sqlConnPool::getInstance()
{
    static sqlConnPool instance;
    return &instance;
}

SqlWrapper::SqlWrapper(MYSQL **conn, sqlConnPool* pool)
{
    *conn = pool->getConn();
    m_conn = *conn;
    m_pool = pool;
}

SqlWrapper::~SqlWrapper()
{
    m_pool->releaseConn(m_conn);
}
