# 添加子目录
add_subdirectory(Locker)
add_subdirectory(webServer)
add_subdirectory(HTTP)
add_subdirectory(Util)
add_subdirectory(Config)
add_subdirectory(Timer)
add_subdirectory(Client)
add_subdirectory(sqlConnPool)
add_subdirectory(Logger)

# 添加可执行文件
add_executable(main main.cpp)

# 包含 threadPool 目录下的头文件
include_directories(threadPool)

# 链接库依赖
target_link_libraries(main webServer HTTP pthread Util sqlConnPool mysqlclient curl Client Timer Config Logger Locker)


