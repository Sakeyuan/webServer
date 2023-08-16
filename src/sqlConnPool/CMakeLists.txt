# webServer/CMakeLists.txt
# 添加 sqlConnPool 库的源文件
set(SOURCES
    sqlConnPool.cpp   
)

# 添加 sqlConnPool 库的头文件
set(HEADERS
    sqlConnPool.h
)

# 编译 sqlConnPool 库
add_library(sqlConnPool ${SOURCES} ${HEADERS})

# 设置 sqlConnPool 头文件的搜索路径
target_include_directories(sqlConnPool PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})


