#include "utils.h"
#include "../Timer/timer_wheel.h"
#include "../webServer/webServer.h"

using std::cout;
using std::endl;

int *Utils::u_pipefd = 0;
int Utils::u_epollfd = 0;

Utils::Utils()
{
    m_timerWheel = new TimerWheel();

    init_log();
    init_sqlConnPool();
}

Utils::~Utils()
{
    delete m_timerWheel;
}

Utils* Utils::getInstance(){
    static Utils instance;
    return &instance;
}

/**
 * @brief 修改epoll上的fd
 * 
 * @param epollfd 
 * @param fd 
 * @param ev 
 * @param trigMode 
 */
void Utils::mod_fd(int epollfd, int fd, int ev, int trigMode)
{
    epoll_event event;
    memset(&event, 0, sizeof(event));
    event.data.fd = fd;
    if(1 == trigMode){
        event.events = ev | EPOLLET | EPOLLONESHOT | EPOLLRDHUP;
    }
    else{
        event.events = ev | EPOLLONESHOT | EPOLLRDHUP;
    }
    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
}

/**
 * @brief 从epollfd上移除fd
 * 
 * @param epollfd 
 * @param fd 
 */
void Utils::remove_fd(int epollfd, int fd)
{
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, 0);
    close(fd);
}

/**
 * @brief 设置文件描述符非阻塞
 *
 * @param fd 文件描述符
 * @return 设置文件描述符非阻塞
 */
int Utils::setnoblocking(int fd)
{
    int old_flag = fcntl(fd, F_GETFL);
    int new_flag = old_flag | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_flag);
    return old_flag;
}

/**
 * @brief 注册内核事件
 *
 * @param epollfd 注册内核事件
 * @param fd 要注册的fd
 * @param one_shot 是否注册EPOLLONESHOT
 * @param TrigMode 是否注册ET模式
 */
void Utils::add_fd(int epollfd, int fd, bool one_shot, int TrigMode)
{
    epoll_event event;
    memset(&event, 0, sizeof(event));
    event.data.fd = fd;
    if (1 == TrigMode)
        event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    else
        event.events = EPOLLIN | EPOLLRDHUP;
    
    if (one_shot)
        event.events |= EPOLLONESHOT;
    
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnoblocking(fd);
}

/**
 * @brief 信号处理
 * 
 * @param signal 
 */
void Utils::sig_handler(int signal)
{
    //为保证函数的可重入性，保留原来的errno
    //可重入性表示中断后再次进入该函数，环境变量与之前相同，不会丢失数据
    int save_errno = errno;
    int msg = signal;
    //传输字符型，信号值的范围比较小，char可以表示
    send(u_pipefd[1],(char*)&msg,1,0);
    errno = save_errno;
}

/**
 * @brief
 *
 * @param signal 注册信号
 * @param handle 信号处理函数
 * @param restart 是否设置SA_RESTART防止系统调用被信号处理中断
 */
void Utils::add_sig(int signal, void (*handler)(int), bool restart)
{
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = handler;
    if (restart)
        sa.sa_flags |= SA_RESTART;

    //信号处理期间，屏蔽其他信号
    sigfillset(&sa.sa_mask);
    if (sigaction(signal, &sa, nullptr) == -1)
    {
        std::cout << "signal sigaction failed";
    }
}

/**
 * @brief 定时处理任务，重新定时不断触发SIGNAL信号
 * 
 */
void Utils::timer_handler(){

    m_timerWheel->tick();
    alarm(m_timerSlot);
}

void Utils::setTimeSlot(int timeout){
    m_timerSlot = timeout;
}

void Utils::reduceClients(){
    m_locker.lock();
    webServer::m_client_nums--;
    m_locker.unlock();
}

int Utils::getClients() const{
     return webServer::m_client_nums;
}

std::unordered_map<std::string, std::string>& Utils::getUsers(){
     return webServer::m_users;
}

void Utils::init_sqlConnPool(){
    try
    {
        m_sqlConnPool = sqlConnPool::getInstance();
    }
    catch (const std::exception &e)
    {
        cout << "数据库连接池内存分配失败" << endl;
        exit(-1);
    }
    getSqlUser();
}

void Utils::getSqlUser(){
    //从数据可连接池获取一个连接
    MYSQL *mysql = nullptr;
    SqlWrapper mysqlConn(&mysql, m_sqlConnPool);

    string sql_select = "select user_name,user_password from user_info";
    if (mysql_query(mysql, sql_select.c_str()) != 0)
    {
        // 在这里，你可以使用 mysql_errno() 获取错误码，mysql_error() 获取错误信息
        printf("MySQL Error %d: %s\n", mysql_errno(mysql), mysql_error(mysql));
        exit(-1); 
    }
    //获取完整结果集
    MYSQL_RES *res = mysql_store_result(mysql);
    
    //获取结果集列数
    int res_nums = mysql_num_fields(res);

    //获取所有字段数组
    MYSQL_FIELD *fields = mysql_fetch_field(res);

    //从结果集中获取下一行，将对应的用户名和密码，存入map中
    while (MYSQL_ROW row = mysql_fetch_row(res))
    {
        string name(row[0]);
        string password(row[1]);
        webServer::m_users[name] = password;
    }
}

/**
 * @brief 向数据库中插入一条记录，并更新users用户表
 * 
 * @param userName 用户名
 * @param userPwd 密码
 * @return true 
 * @return false 
 */
bool Utils::sqlInsertNP(string userName,string userPwd){
    if(userName.size() == 0 || userPwd.size() == 0){
        return false;
    }
    MYSQL *mysql = nullptr;
    SqlWrapper mysqlConn(&mysql, m_sqlConnPool);
    string sql_insert = "insert into user_info(user_name,user_password) values('" + userName + "','" + userPwd + "')";
    if(mysql_query(mysql,sql_insert.c_str())){
        printf("MySQL Error %d: %s\n", mysql_errno(mysql), mysql_error(mysql));
        return false;
    }
    m_locker.lock();
    webServer::m_users[userName] = userPwd;
    m_locker.unlock();
    return true;
}

std::string Utils::getCurTime(std::string format){
    struct tm tm;
    time_t time = std::time(nullptr);
    localtime_r(&time, &tm);
    char buf[64];
    strftime(buf, sizeof(buf), format.c_str(), &tm);
    return string(buf);
}

std::string Utils::getCurPath(){
    char curPath[200];
    getcwd(curPath,200);
    char *lastpos = strrchr(curPath, '/');
    *lastpos = '\0';
    return string(curPath);
}

void Utils::init_log(){
    LogMgr = LoggerManager::getInstance();
    string date = getCurTime();
    string logPath = date + "-webServer.log";
    std::string curPath = getCurPath();
    curPath += "/" + logPath;
    m_logger = LogMgr->getLogger("webServer");
    m_logger->addAppender(LogAppender::ptr(new StdoutLogAppender));
    m_logger->addAppender(LogAppender::ptr(new FileLogAppender(curPath)));
}