# sudo apt-get install libmysqlclient-dev
# mysql.h 头文件以及动态库和静态库

cmake_minimum_required(VERSION 3.23)

project(dbframe)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/")

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(fmt)

add_subdirectory(dbimpl)

add_subdirectory(example/sync)
add_subdirectory(example/async)