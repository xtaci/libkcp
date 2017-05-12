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
@protocol SFKcpTunDelegate <NSObject>
-(void)connected:(SFKcpTun*)tun;
-(void)disConnected:(SFKcpTun*)tun;
-(void)tunError:(SFKcpTun*)tun error:(NSError*)error;
-(void)didRecevied:(NSData*)data;
@end
@interface SFKcpTun : NSObject
@property (strong,nonatomic) TunConfig *config;
@property (strong,nonatomic) NSString *server;
@property (weak,nonatomic) NSObject<SFKcpTunDelegate> *delegate;
@property (nonatomic) int port;
@property (nonatomic) BOOL connected;

@property (nonatomic)  dispatch_queue_t dispatchqueue ;
-(instancetype)initWithConfig:(TunConfig *)c ipaddr:(NSString*)ip port:(int)port queue:(dispatch_queue_t)dqueue;

-(void)startUDPSession;
-(void)restartUDPSessionWithIpaddr:(NSString*)ip port:(int)port;
-(void)shutdownUDPSession;
-(void)input:(NSData*)data;

@end
