//
//  SFKcpTun.m
//  libkcp
//
//  Created by 孔祥波 on 28/04/2017.
//  Copyright © 2017 Kong XiangBo. All rights reserved.
//
#include <unistd.h>
#include <sys/time.h>
#include <cstring>
#include <cstdio>
#include "sess.h"
#import "SFKcpTun.h"
UDPSession *sess;

void
itimeofday(long *sec, long *usec) {
    struct timeval time;
    gettimeofday(&time, NULL);
    if (sec) *sec = time.tv_sec;
    if (usec) *usec = time.tv_usec;
}

IUINT64 iclock64(void) {
    long s, u;
    IUINT64 value;
    itimeofday(&s, &u);
    value = ((IUINT64) s) * 1000 + (u / 1000);
    return value;
}

IUINT32 iclock() {
    return (IUINT32) (iclock64() & 0xfffffffful);
}
@interface SFKcpTun ()
//@property (weak, nonatomic)  UDPSession *sess;
//@property (strong, nonatomic)  UITextField *port;

@end

@implementation SFKcpTun
-(instancetype)initWithConfig:(TunConfig *)c ipaddr:(NSString*)ip port:(int)port
{
    if (self = [super init]){
        self.config = c;
        self.server = ip;
        self.port = port;
        [self startUDPSession];
    }
    return self;
}
-(void)startUDPSession
{
    sess = UDPSession::DialWithOptions(self.server.UTF8String, self.port, self.config.dataShards,self.config.parityShards);
    sess->NoDelay(self.config.nodelay, self.config.interval, self.config.resend, self.config.nc);
    sess->WndSize(self.config.sndwnd, self.config.rcvwnd);
    sess->SetMtu(self.config.mtu);
    sess->SetStreamMode(true);
    sess->SetDSCP(self.config.iptos);
    
    assert(sess != nullptr);
    [self run];
}
-(void)restartUDPSessionWithIpaddr:(NSString*)ip port:(int)port
{
    if (sess != nil) {
        UDPSession::Destroy(sess);
    }
    sess = UDPSession::DialWithOptions(ip.UTF8String, port, self.config.dataShards,self.config.parityShards);
    sess->NoDelay(self.config.nodelay, self.config.interval, self.config.resend, self.config.nc);
    sess->WndSize(self.config.sndwnd, self.config.rcvwnd);
    sess->SetMtu(self.config.mtu);
    sess->SetStreamMode(true);
    sess->SetDSCP(self.config.iptos);
    
    assert(sess != nullptr);
    [self run];
}
-(void)shutdownUDPSession
{
    UDPSession::Destroy(sess);

}
-(void)input:(NSData*)data{
    
    assert(sess != nullptr);

    sess->Write((char *)data.bytes, data.length);
    sess->Update(iclock());
}
-(void)run{
    //不能一直读,需要timer 
    dispatch_queue_t q = dispatch_queue_create("com.abigt.kcp", nil);
    dispatch_async(q, ^{
        for(;;){
            if (sess == nil) {
                break;
            }
            char *buf = (char *) malloc(4096);
            memset(buf, 0, 4096);
            ssize_t n = sess->Read(buf, 4096);
            sess->Update(iclock());
            if (n > 0 ){
                NSData *d = [NSData dataWithBytes:buf length:n];
                [self.delegate didRecevied:d];
            }
            free(buf);
        }
       
    });
}
@end
