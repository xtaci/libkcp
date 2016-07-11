cmake_minimum_required(VERSION 3.5)
project(kcp)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES main.cpp ikcp.c ikcp.h sess.cpp sess.h io.h)
add_executable(kcp ${SOURCE_FILES})