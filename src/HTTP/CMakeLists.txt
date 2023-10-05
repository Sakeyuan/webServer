# HTTP/CMakeLists.txt
# 添加 HTTP 库的源文件
set(SOURCES
    Http.cpp   
)

# 添加 HTTP 库的头文件
set(HEADERS
    Http.h
    parse_status.h
)

# 编译 HTTP 库
add_library(HTTP ${SOURCES} ${HEADERS})

# 设置 HTTP 头文件的搜索路径
target_include_directories(HTTP PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})


