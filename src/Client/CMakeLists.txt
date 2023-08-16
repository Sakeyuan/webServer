# 添加 Config 库的源文件
set(SOURCES
    client.cpp
)

# 添加 Config 库的头文件
set(HEADERS
    client.h
)

# 编译 Config 库
add_library(Client ${SOURCES} ${HEADERS})

# 设置 Config 头文件的搜索路径
target_include_directories(Client PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})


