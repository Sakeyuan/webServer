# HTTP/CMakeLists.txt
# 添加 HTTP 库的源文件
set(SOURCES
    locker.cpp   
)

# 添加 HTTP 库的头文件
set(HEADERS
    locker.h
)

# 编译 HTTP 库
add_library(Locker ${SOURCES} ${HEADERS})

# 设置 HTTP 头文件的搜索路径
target_include_directories(Locker PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})


