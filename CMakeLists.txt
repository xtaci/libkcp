cmake_minimum_required(VERSION 2.8.7)
project(kcp)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES main.cpp ikcp.c ikcp.h sess.cpp sess.h io.h galois.h galois.c)
add_executable(kcp ${SOURCE_FILES})
