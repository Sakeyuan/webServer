# 添加 Config 库的源文件
set(SOURCES
    Config.cpp
)

# 添加 Config 库的头文件
set(HEADERS
    Config.h
)

# 编译 Config 库
add_library(Config ${SOURCES} ${HEADERS})

# 设置 Config 头文件的搜索路径
target_include_directories(Config PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})


