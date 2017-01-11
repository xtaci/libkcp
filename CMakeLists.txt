cmake_minimum_required(VERSION 2.8.7)
project(kcp)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(MAIN_TEST kcp_test.cpp)
set(FEC_TEST fec_test.cpp)
set(SOURCE_FILES ikcp.c sess.cpp galois.cpp galois_noasm.cpp matrix.cpp inversion_tree.cpp reedsolomon.cpp fec.cpp galois_table.c)
add_executable(kcp_test ${SOURCE_FILES} ${MAIN_TEST})
add_executable(fec_test ${SOURCE_FILES} ${FEC_TEST})
