//
//  BlockCrypt.cpp
//  libkcp
//
//  Created by 孔祥波 on 04/05/2017.
//  Copyright © 2017 Kong XiangBo. All rights reserved.
//

#include "BlockCrypt.h"
#import <Security/Security.h>
#include <Security/SecRandom.h>
const  uint8_t iv[] =  {167, 115, 79, 156, 18, 172, 27, 1, 164, 21, 242, 193, 252, 120, 230, 107};
BlockCrypt*
BlockCrypt::blockWith(const void* key,const char* crypto){
    if (strcmp(crypto, "none") == 0) {
        return NULL;
    }
    BlockCrypt *block = new (BlockCrypt);
    
    
    const void *ivPtr = iv;
    size_t keyLen = 0;
    if (strcmp(crypto,"aes") == 0){
        keyLen = 32;
    }else if (strcmp(crypto,"aes-128") == 0){
        keyLen = 16;
    }else {
        keyLen = 24;
    }

   
    CCCryptorStatus st = CCCryptorCreateWithMode(
                            kCCEncrypt,
                            kCCModeCFB,
                            kCCAlgorithmAES,
                            ccNoPadding,
                            ivPtr, key, keyLen,
                            NULL, 0, 0, 0,
                            &(block->send_ctx));
  
    if (st != kCCSuccess){
        printf("send_ctx create error\n");
    }
    st =  CCCryptorCreateWithMode(
                                  kCCDecrypt,
                                  kCCModeCFB,
                                  kCCAlgorithmAES,
                                  ccNoPadding,
                                  ivPtr, key, keyLen,
                                  NULL, 0, 0, 0,
                                  &(block->recv_ctx));
    if (st != kCCSuccess){
        printf("recv_ctx create error \n");
    }
    
    return block;
    
}
// output udp packet
void 
BlockCrypt::encrypt(void *buffer, size_t length,size_t *outlen)
{
    
    size_t len = length + kCCBlockSizeAES128;
    
    void *dataOut = malloc(len);
    
    CCCryptorStatus st  = CCCryptorUpdate(send_ctx, buffer, length, dataOut, len, outlen);
    if (st != kCCSuccess){
        printf("encrypt data error");
       
    }else {
        memcpy(buffer, dataOut, *outlen);
        
    }
    free(dataOut);
}
void
BlockCrypt::decrypt(void *buffer, size_t length,size_t *outlen)
{
    size_t len = length + kCCBlockSizeAES128;
    
    void *dataOut = malloc(len);
    
    CCCryptorStatus st  = CCCryptorUpdate(recv_ctx, buffer, length, dataOut, len, outlen);
    if (st != kCCSuccess){
        printf("encrypt data error");
        
    }else {
        memcpy(buffer, dataOut, *outlen);
    }
    free(dataOut);
}
void
BlockCrypt::Destroy(BlockCrypt *block) {
    if (block->send_ctx != NULL) {
        CCCryptorRelease(block->send_ctx);
    }
    if (block->recv_ctx != NULL) {
        CCCryptorRelease(block->recv_ctx);
    }
    delete block;
}
uint8_t *
BlockCrypt::ramdonBytes(size_t len){
    
    uint8_t *ptr = (uint8_t *)malloc(len);
    // Gen random bytes
    if  (SecRandomCopyBytes(kSecRandomDefault, len, (uint8_t *)ptr) == 0){
        return ptr;
    }
    free(ptr);
    return nil;
}
