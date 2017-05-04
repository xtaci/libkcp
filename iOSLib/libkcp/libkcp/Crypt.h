//
//  BlockCrypt.h
//  libkcp
//
//  Created by 孔祥波 on 04/05/2017.
//  Copyright © 2017 Kong XiangBo. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <CommonCrypto/CommonCrypto.h>
@interface Crypt : NSObject
@property (strong,nonatomic) NSData *initialVector;
@property (nonatomic) CCCryptorRef *send_ctx;
@property (nonatomic) CCCryptorRef *recv_ctx;
-(instancetype)initWithKey:(NSData*)key crypto:(NSString*)crypto;
-(NSData*)encrypt:(NSData*)data;
-(NSData*)decrypt:(NSData*)data;
@end
