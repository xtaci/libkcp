//
//  BlockCrypt.m
//  libkcp
//
//  Created by 孔祥波 on 04/05/2017.
//  Copyright © 2017 Kong XiangBo. All rights reserved.
//

#import "Crypt.h"

static NSString *saltxor = @"sH3CIVoF#rWLtJo6";
@implementation Crypt
-(instancetype)initWithKey:(NSData*)key crypto:(NSString*)crypto{
    if (self = [super init]){
        uint8_t iv[] =  {167, 115, 79, 156, 18, 172, 27, 1, 164, 21, 242, 193, 252, 120, 230, 107};
        self.initialVector = [NSData dataWithBytes:iv length:16];
        const void *ckey = key.bytes;
        const void *ivPtr = iv;
        size_t keyLen = 0;
        if ([crypto isEqualToString:@"aes"]){
            keyLen = 32;
        }else if ([crypto isEqualToString:@"aes-128"]){
            keyLen = 16;
        }else {
            keyLen = 24;
        }
        CCCryptorRef s = self.send_ctx;
        CCCryptorStatus st = CCCryptorCreate(kCCEncrypt, kCCAlgorithmDES, 0, ckey, keyLen, ivPtr, &s);
        if (st != kCCSuccess){
            NSLog(@"send_ctx create error");
        }
        CCCryptorRef r = self.send_ctx;
        st = CCCryptorCreate(kCCDecrypt, kCCAlgorithmDES, 0, ckey, keyLen, ivPtr, &r);
        if (st != kCCSuccess){
            NSLog(@"recv_ctx create error");
        }
    }
    return self;
}
-(NSData*)encrypt:(NSData*)data
{
    const void *dataIn = data.bytes;
    size_t len = data.length + kCCBlockSizeAES128;
    int outLen = 0;
    NSMutableData *o = [NSMutableData dataWithLength:len];
    void *dataOut =(void *) o.bytes;
    CCCryptorStatus st  = CCCryptorUpdate(self.send_ctx, dataIn, data.length, dataOut, len, &len);
    if (st != kCCSuccess){
        NSLog(@"encrypt data error");
        return  nil;
    }else {
        o.length = outLen;
        return o;
    }
}
-(NSData*)decrypt:(NSData*)data
{
    const void *dataIn = data.bytes;
    size_t len = data.length + kCCBlockSizeAES128;
    int outLen = 0;
    NSMutableData *o = [NSMutableData dataWithLength:len];
    void *dataOut =(void *) o.bytes;
    CCCryptorStatus st  = CCCryptorUpdate(self.recv_ctx, dataIn, data.length, dataOut, len, &len);
    if (st != kCCSuccess){
        NSLog(@"encrypt data error");
        return  nil;
    }else {
        o.length = outLen;
        return o;
    }
}
-(void)dealloc
{
    CCCryptorRelease(self.send_ctx);
    CCCryptorRelease(self.recv_ctx);
    //super.dealloc();
}
@end
