# webServer/CMakeLists.txt
# 添加 webServer 库的源文件
set(SOURCES
    log.cpp
)

# 添加 webServer 库的头文件
set(HEADERS
    log.h
)

# 编译 webServer 库
add_library(Logger ${SOURCES} ${HEADERS})

# 设置 webServer 头文件的搜索路径
target_include_directories(Logger PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})


