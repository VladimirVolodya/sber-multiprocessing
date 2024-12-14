cmake_minimum_required(VERSION 3.30)

project(pbst)

set(CMAKE_CXX_STANDARD 20)

include_directories(include)

# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DDEBUG")

add_executable(pbst main.cpp)
