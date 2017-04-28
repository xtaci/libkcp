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

@end
