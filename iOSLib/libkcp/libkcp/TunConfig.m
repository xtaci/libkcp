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
        self.dataShards = 2;
        self.parityShards = 2;
        
        self.nodelay = 1;
        self.interval = 20;
        self.resend = 2;
        self.nc = 1;
        
        self.resend = 128;
        self.rcvwnd = 128;
        self.mtu = 1400 ;
        self.iptos = 46;
        
    }
    return  self;
}
@end
