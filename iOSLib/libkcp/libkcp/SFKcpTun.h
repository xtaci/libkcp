//
//  SFKcpTun.h
//  libkcp
//
//  Created by 孔祥波 on 28/04/2017.
//  Copyright © 2017 Kong XiangBo. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "TunConfig.h"
@class SFKcpTun;

typedef void (^tunConnected)(SFKcpTun * _Nonnull tun);
typedef void (^tunDisConnected)(SFKcpTun * _Nonnull tun);
typedef void (^tundidFoundError)(SFKcpTun * _Nonnull tun  ,NSError * _Nonnull e);
typedef void (^didRecvData)(SFKcpTun * _Nonnull tun,NSData * _Nonnull d);

@interface SFKcpTun : NSObject
    @property (strong,nonatomic) TunConfig * _Nonnull config;
    @property (strong,nonatomic) NSString * _Nonnull server;

@property (nonatomic) int port;
@property (nonatomic) BOOL connected;
    
@property (nonatomic,copy) didRecvData _Nonnull recvData;
@property (nonatomic,copy) tunDisConnected _Nonnull disConnected;
@property (nonatomic,copy) tunConnected _Nonnull tunConnected;
//callback queue
    @property (nonatomic)  dispatch_queue_t _Nonnull dispatchqueue ;
-(instancetype _Nonnull )initWithConfig:(TunConfig *_Nonnull)c ipaddr:(NSString*_Nonnull)ip port:(int)port queue:(dispatch_queue_t _Nonnull )dqueue;
-(void)startWith:(tunConnected _Nonnull )connectd recv:(didRecvData _Nonnull )recv disConnect:(tunConnected _Nonnull )disConnect;
-(void)startUDPSession;
-(void)restartUDPSessionWithIpaddr:(NSString*_Nonnull)ip port:(int)port;
-(void)shutdownUDPSession;
-(void)input:(NSData*_Nonnull)data;
//-(void)upDate;
-(NSString*_Nonnull)localAddress;
-(BOOL)useCell;
@end
