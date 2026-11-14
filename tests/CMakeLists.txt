add_executable(kcp_test kcp_test.c)
target_link_libraries(kcp_test kcp)

add_executable(fec_test fec_test.c)
target_link_libraries(fec_test kcp)
add_test(NAME fec COMMAND fec_test)

add_executable(autotune_test autotune_test.c)
target_link_libraries(autotune_test kcp)
add_test(NAME autotune COMMAND autotune_test)

add_executable(reedsolomon_test reedsolomon_test.c)
target_link_libraries(reedsolomon_test kcp)
add_test(NAME reedsolomon COMMAND reedsolomon_test)

if(KCP_ENABLE_CRYPTO)
    add_executable(crypt_test crypt_test.c)
    target_link_libraries(crypt_test kcp)
    add_test(NAME crypt COMMAND crypt_test)
endif()
