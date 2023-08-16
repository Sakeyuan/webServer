# 添加 Config 库的源文件
set(SOURCES
    timer_wheel.cpp
)

# 添加 Config 库的头文件
set(HEADERS
    timer_wheel.h
)

# 编译 Config 库
add_library(Timer ${SOURCES} ${HEADERS})

# 设置 Config 头文件的搜索路径
target_include_directories(Timer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})


