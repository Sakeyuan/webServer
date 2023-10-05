cmake_minimum_required(VERSION 3.0)
project(MyProject)

# 使用C++11标准
set(CMAKE_CXX_STANDARD 11)

# 添加DEBUG信息  
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")

# 设置输出二进制文件的路径
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# 添加子目录
add_subdirectory(src)