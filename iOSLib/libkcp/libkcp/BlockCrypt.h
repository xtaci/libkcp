//
//  BlockCrypt.hpp
//  libkcp
//
//  Created by 孔祥波 on 04/05/2017.
//  Copyright © 2017 Kong XiangBo. All rights reserved.
//

#ifndef BlockCrypt_hpp
#define BlockCrypt_hpp
#import <CommonCrypto/CommonCrypto.h>
#include <stdio.h>
class BlockCrypt  {
private:
    CCCryptorRef *send_ctx;
    CCCryptorRef *recv_ctx;
public:
    static BlockCrypt* blockWith(const void* key,const char* crypt);
    // output udp packet
    char *encrypt(const void *buffer, size_t length,size_t *outlen);
    char *decrypt(const void *buffer, size_t length,size_t *outlen);
private:
    BlockCrypt() = default;
    
    ~BlockCrypt() = default;
    static void Destroy(BlockCrypt *block);
};
#endif /* BlockCrypt_hpp */
