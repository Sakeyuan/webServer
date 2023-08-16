# webServer

    --项目目录结构
    ├── build
    ├── CMakeLists.txt
    ├── Config.conf
    ├── README.md
    ├── root
    │   ├── images
    │   │   └── imag.jpg
    │   ├── index.html
    │   ├── rabbit.mp4
    │   └── welcome.html
    └── src
        ├── Client
        │   ├── client.cpp
        │   ├── client.h
        │   └── CMakeLists.txt
        ├── CMakeLists.txt
        ├── Config
        │   ├── CMakeLists.txt
        │   ├── Config.cpp
        │   └── Config.h
        ├── HTTP
        │   ├── CMakeLists.txt
        │   ├── Http.cpp
        │   ├── Http.h
        │   └── parse_status.h
        ├── Locker
        │   ├── CMakeLists.txt
        │   ├── locker.cpp
        │   └── locker.h
        ├── main.cpp
        ├── test
        ├── test.cpp
        ├── threadPool
        │   └── thread_pool.h
        ├── Timer
        │   ├── CMakeLists.txt
        │   ├── timer_list.cpp
        │   └── timer_list.h
        ├── Util
        │   ├── CMakeLists.txt
        │   ├── utils.cpp
        │   └── utils.h
        └── webServer
            ├── CMakeLists.txt
            ├── webServer.cpp
            └── webServer.h

<br><br>
# 主要运用技术：

    线程池 + 数据连接池 + 模拟同步I/O的proactor模式 + epoll ET 和 LT模式 + HTTP报文处理状态机 + 时间轮定时器 + C++类日志系统


<br><br>

# 项目简介（参考了开源项目TinyWebServer）

 服务器是一个 HTTP 服务器，主线程可以使用 epoll 的 ET 和 LT 模式通过监听服务器指定的 socket 是否有事件发生，有事件发生 epoll_wait 通知主线程，如果事件是客户端的连接，就将用户的信息保存起来；如果事件是可读事件或者可写事件，使用的是同步 I/O 方式模拟 Proactor 模式，主线程负责读和写相应 socket 的数据，将浏览器发送过来的数据读取到 m_read_buf 读缓冲区中，读取完成后，主线程通过 add() 方法将请求添加到队列中，主线程和所有子线程通过共享一个请求队列 m_work_queue 来同步，并用互斥锁保证线程安全。子线程都睡眠在请求队列上，通过信号量控制。定时器使用时间轮算法，处理非活跃连接，日志系统，服务器可以达到 20000 多并发。



<br><br>

# 构建方式

    进入build，在终端使用命令make,运行main程序

    数据库、ET、LT等配置在配置文件Config.conf文件写入

<div>
  <h2>日志系统类图</h2>
  <br>
  <img src="./root/images/3.png">
</div>

<br>

# 测试结果
<div>
  <h2>proactor LT + LT</h2>
  <br>
  <img src="./root/images/1.png" >
</div>
<br><br>
<div>
  <h2>proactor ET + ET</h2>
  <br>
  <img src="./root/images/2.png">
</div>

<br>
<div>
  <h2>登录</h2>
  <br>
  <img src="./root/images/6.png">
  <br><br>
  <h2>注册</h2>
  <br>
  <img src="./root/images/4.png">
    <br><br>
  <h2>主页</h2>
  <br>
  <img src="./root/images/5.png">
</div>
 


