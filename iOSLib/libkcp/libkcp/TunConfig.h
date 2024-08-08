//
//  TunConfig.h
//  libkcp
//
//  Created by 孔祥波 on 28/04/2017.
//  Copyright © 2017 Kong XiangBo. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface TunConfig : NSObject
//DialWithOptions
@property (nonatomic) int dataShards;
@property (nonatomic) int parityShards;

@property (nonatomic) int nodelay;
@property (nonatomic) int interval;
@property (nonatomic) int resend;
@property (nonatomic) int nc;

@property (nonatomic) int sndwnd;
@property (nonatomic) int rcvwnd;

@property (nonatomic) int mtu;

@property (nonatomic) int iptos;

@property (nonatomic) int keepAliveInterval;
@property (nonatomic) int keepAliveTimeout;
@property (strong,nonatomic) NSData *key; //pkbdf2Key driven
//"aes, aes-128, aes-192, salsa20, blowfish, twofish, cast5, 3des, tea, xtea, xor, none",
@property (strong,nonatomic) NSString *crypt;
@end
