# HTTP/CMakeLists.txt
# 添加 HTTP 库的源文件
set(SOURCES
    webServer.cpp   
)

# 添加 HTTP 库的头文件
set(HEADERS
    webServer.h
)

# 编译 HTTP 库
add_library(webServer ${SOURCES} ${HEADERS})

# 设置 HTTP 头文件的搜索路径
target_include_directories(webServer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})


