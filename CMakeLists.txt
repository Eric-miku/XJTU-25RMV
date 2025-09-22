cmake_minimum_required(VERSION 3.10)

# 工程名字
project(RMV02)

# 设置 C++ 标准（例如 C++11）
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 找到 OpenCV
find_package(OpenCV REQUIRED)

# 生成可执行文件
add_executable(main src/main.cpp)
# 链接 OpenCV
target_link_libraries(main ${OpenCV_LIBS})