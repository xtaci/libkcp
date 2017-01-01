cmake_minimum_required(VERSION 2.8.7)
project(kcp)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES main.cpp ikcp.c sess.cpp galois.c galois_noasm.c matrix.cpp inversion_tree.cpp reedsolomon.cpp)
add_executable(kcp ${SOURCE_FILES})
