//
//  TunConfig.m
//  libkcp
//
//  Created by 孔祥波 on 28/04/2017.
//  Copyright © 2017 Kong XiangBo. All rights reserved.
//

#import "TunConfig.h"

@implementation TunConfig

- (instancetype)init{
    if (self = [super init]){
        self.dataShards = 10;
        self.parityShards = 3;
        
        self.nodelay = 0;
        self.interval = 30;
        self.resend = 2;
        self.nc = 1;
        
        self.resend = 128;
        self.rcvwnd = 128;
        self.mtu = 1350 ;
        self.iptos = 46;
        self.crypt = @"none";
    }
    return  self;
}
@end
