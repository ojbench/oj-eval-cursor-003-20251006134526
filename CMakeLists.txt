cmake_minimum_required(VERSION 3.10)
project(ICPC_System)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")

add_executable(code main.cpp)
