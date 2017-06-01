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
{
    dispatch_source_t _timer;
    dispatch_queue_t queue ;
    dispatch_queue_t socketqueue ;
    
}
-(instancetype)initWithConfig:(TunConfig *)c ipaddr:(NSString*)ip port:(int)port queue:(dispatch_queue_t)dqueue
{
    if (self = [super init]){
        self.config = c;
        self.server = ip;
        self.port = port;
        self.dispatchqueue = dqueue;
        queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
        socketqueue = dispatch_queue_create("com.abigt.kcpwrite", DISPATCH_QUEUE_SERIAL);
        [self startUDPSession];
    }
    return self;
}
-(void)startUDPSession
{
    if (self.config.key.length > 0){
        
        BlockCrypt *block = BlockCrypt::blockWith(self.config.key.bytes, self.config.crypt.UTF8String);
        sess = UDPSession::DialWithOptions(self.server.UTF8String, self.port, self.config.dataShards,self.config.parityShards,block);
    }else {
        sess = UDPSession::DialWithOptions(self.server.UTF8String, self.port, self.config.dataShards,self.config.parityShards);
    }
    
    sess->NoDelay(self.config.nodelay, self.config.interval, self.config.resend, self.config.nc);
    sess->WndSize(self.config.sndwnd, self.config.rcvwnd);
    sess->SetMtu(self.config.mtu);
    sess->SetStreamMode(true);
    sess->SetDSCP(self.config.iptos);
    assert(sess != nullptr);
    self.connected = true;
    [self runTest];
    
}
-(void)restartUDPSessionWithIpaddr:(NSString*)ip port:(int)port
{
    if (sess != nil) {
        UDPSession::Destroy(sess);
    }
    if (self.config.key.length > 0){
        
        BlockCrypt *block = BlockCrypt::blockWith(self.config.key.bytes, self.config.crypt.UTF8String);
        sess = UDPSession::DialWithOptions(self.server.UTF8String, self.port, self.config.dataShards,self.config.parityShards,block);
    }else {
        sess = UDPSession::DialWithOptions(self.server.UTF8String, self.port, self.config.dataShards,self.config.parityShards);
    }
    
  
    sess->NoDelay(self.config.nodelay, self.config.interval, self.config.resend, self.config.nc);
    sess->WndSize(self.config.sndwnd, self.config.rcvwnd);
    sess->SetMtu(self.config.mtu);
    sess->SetStreamMode(true);
    sess->SetDSCP(self.config.iptos);
    self.connected = true;
    assert(sess != nullptr);
    [self runTest];
}
-(void)shutdownUDPSession
{
    self.connected = false;
    UDPSession::Destroy(sess);

}

-(void)input:(NSData*)data{
    
    assert(sess != nullptr);

    __weak  SFKcpTun *weakSelf = self;
    dispatch_async(socketqueue, ^{
        SFKcpTun* strongSelf = weakSelf;
        
        size_t tosend =  data.length;
        size_t sended = 0 ;
        char *ptr = (char *)data.bytes;

        if  (strongSelf.connected) {
            while (sended < tosend) {
                
                
                size_t sendt = sess->Write(ptr, data.length - sended);
                sended += sendt ;
                ptr += sended;
                //NSLog(@"KCPTun sended:%zu, totoal:= %zu",sended,tosend);
                //不能并行接收网络数据,效率有折扣
                //sess->Update(iclock());
            }
            if (sended == [data length]) {
                NSLog(@"KCPTun sent %zu",sended);
            }else {
                exit(-1);
            }
        }
        
        
    });
  
    
    
}

-(void)runTest
{   __weak  SFKcpTun *weakSelf = self;
    dispatch_async(self->queue, ^{
        size_t total = 0;
        NSDate *start = [NSDate date];
        SFKcpTun* strongSelf = weakSelf;
        while (strongSelf.connected) {
            
            if (total == 0) {
                start = [NSDate date];
            }
            if (strongSelf) {
                if (sess != nil) {
                    
                    
                    char *buf = (char *) malloc(4096);
                    
                    memset(buf, 0, 4096);
                    ssize_t n = sess->Read(buf, 4096);
                    sess->Update(iclock());
                    
                    if (n > 0 ){
                        @autoreleasepool {
                            NSData *d = [NSData dataWithBytes:buf length:n];
                            
                            dispatch_async(strongSelf.dispatchqueue, ^{
                                [strongSelf.delegate didRecevied:d];
                            });
                            
                            total += n ;
                            NSDate *now = [NSDate date];
                            
                            //size_t speed =  (NSTimeInterval)total/inter;
//                            if(( [now timeIntervalSinceDate:start]> 3.0 )&&( total > 1024*1000*10))  {
//                                total = 0;
//                                usleep(3300);
//                            }else {
//                                if ( [now timeIntervalSinceDate:start]> 3.0 ){
//                                    total = 0 ;
//                                }
//                                usleep(800);
//                            }

                        }
                        
                        
                    }else {
                        usleep(11000);
                    }
                    free(buf);
                    
                    
                    
                }
            }
            
        }
    });
}
-(void)testPorformce
{
    __weak  SFKcpTun *weakSelf = self;
    dispatch_async(self->queue, ^{
        SFKcpTun* strongSelf = weakSelf;
        while (strongSelf.connected) {
            
            
            if (strongSelf) {
                if (sess != nil) {
                    
                    
                    @autoreleasepool {
                        char  *ptr = (char  *)BlockCrypt::ramdonBytes(1326);
                        
                        size_t sendt = sess->Write(ptr, 1326);
                        free(ptr);

                    }
                    
                    
                    char *buf = (char *) malloc(4096);
                    
                    memset(buf, 0, 4096);
                    ssize_t n = sess->Read(buf, 4096);
                    sess->Update(iclock());
                    
                    if (n > 0 ){
                        @autoreleasepool {
                            NSData *d = [NSData dataWithBytes:buf length:n];
                            
                            dispatch_async(strongSelf.dispatchqueue, ^{
                                [strongSelf.delegate didRecevied:d];
                            });
                        }
                        
                        
                    }else {
                        //NSLog(@"##### kcp recv  null\n");
                    }
                    free(buf);
                    if (n != 1326) {
                        usleep(330);
                    }
                    
                }
            }
            
        }
    });
    
}
@end
