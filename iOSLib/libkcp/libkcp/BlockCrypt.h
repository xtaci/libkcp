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
    CCCryptorRef send_ctx;
    CCCryptorRef recv_ctx;
public:
    static BlockCrypt* blockWith(const void* key,const char* crypt);
    // output udp packet
    void encrypt(void *buffer, size_t length,size_t *outlen);
    void decrypt(void *buffer, size_t length,size_t *outlen);
    uint8_t *ramdonBytes(size_t len);
private:
    BlockCrypt() = default;
    
    ~BlockCrypt() = default;
    static void Destroy(BlockCrypt *block);
};
#endif /* BlockCrypt_hpp */
