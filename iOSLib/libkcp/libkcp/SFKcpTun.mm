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

#include <arpa/inet.h>

#include <sys/socket.h>

#include <ifaddrs.h>
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
-(instancetype)initWithConfig:(TunConfig *)c ipaddr:(NSString*)ip port:(int)port queue:(dispatch_queue_t)dqueue delegate:(NSObject<SFKcpTunDelegate>*)delegate
{
    if (self = [super init]){
        self.config = c;
        self.server = ip;
        self.port = port;
        self.delegate = delegate;
        self.dispatchqueue = dqueue;
        queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
        socketqueue = dispatch_queue_create("com.abigt.kcpwrite", DISPATCH_QUEUE_SERIAL);
        [self startUDPSession];
    }
    return self;
}
-(void)startUDPSession
{
    if (self.config.key.length > 0 && ![self.config.crypt isEqualToString:@"none"]){
        
        BlockCrypt *block = BlockCrypt::blockWith(self.config.key.bytes, self.config.crypt.UTF8String);
        sess = UDPSession::DialWithOptions(self.server.UTF8String, self.port, self.config.dataShards,self.config.parityShards,block);
    }else {
        sess = UDPSession::DialWithOptions(self.server.UTF8String, self.port, self.config.dataShards,self.config.parityShards);
    }
    assert(sess != nullptr);
    
    sess->NoDelay(self.config.nodelay, self.config.interval, self.config.resend, self.config.nc);
    sess->WndSize(self.config.sndwnd, self.config.rcvwnd);
    sess->SetMtu(self.config.mtu);
    sess->SetStreamMode(true);
    sess->SetDSCP(self.config.iptos);
   
    self.connected = true;
    [self runTest];
    __weak  SFKcpTun *weakSelf = self;
    dispatch_async(self.dispatchqueue, ^{
         [weakSelf.delegate connected:weakSelf];
    });
   
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
    
    assert(sess != nullptr);
    sess->NoDelay(self.config.nodelay, self.config.interval, self.config.resend, self.config.nc);
    sess->WndSize(self.config.sndwnd, self.config.rcvwnd);
    sess->SetMtu(self.config.mtu);
    sess->SetStreamMode(true);
    sess->SetDSCP(self.config.iptos);
    self.connected = true;
   
    
    [self runTest];
    __weak  SFKcpTun *weakSelf = self;
    dispatch_async(self.dispatchqueue, ^{
        [weakSelf.delegate connected:weakSelf];
    });
}
-(void)runDispatchTimer
{
    // Create a dispatch source that'll act as a timer on the concurrent queue
    // You'll need to store this somewhere so you can suspend and remove it later on
    dispatch_source_t dispatchSource = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0,self->queue);
    
    // Setup params for creation of a recurring timer
    double interval = 3300.0;
    dispatch_time_t startTime = dispatch_time(DISPATCH_TIME_NOW, 0);
    uint64_t intervalTime = (int64_t)(interval * NSEC_PER_MSEC);
    dispatch_source_set_timer(dispatchSource, startTime, intervalTime, 0);
    
    // Attach the block you want to run on the timer fire
    dispatch_source_set_event_handler(dispatchSource, ^{
        // Your code here
        
        
        if (sess != nil) {
            
            
            char *buf = (char *) malloc(4096);
            
            memset(buf, 0, 4096);
            ssize_t n = sess->Read(buf, 4096);
            sess->Update(iclock());
            
            if (n > 0 ){
                @autoreleasepool {
                    NSData *d = [NSData dataWithBytes:buf length:n];
                    
                    dispatch_async(self.dispatchqueue, ^{
                        [self.delegate didRecevied:d];
                    });
                    
                    // total += n ;
                    //NSDate *now = [NSDate date];
                    
                    
                    
                }
            }
            free(buf);
            
            
            
        }
    });
    
    // Start the timer
    dispatch_resume(dispatchSource);
    
    // ----
    
    // When you want to stop the timer, you need to suspend the source
    //dispatch_suspend(dispatchSource);
    
    // If on iOS5 and/or using MRC, you'll need to release the source too
    
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
            int zeroCount = 0;
            int valueCont = 0 ;
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
                            //NSDate *now = [NSDate date];

                        }

#include "TargetConditionals.h"
#if TARGET_IPHONE_SIMULATOR
                            // iOS Simulator
#elif TARGET_OS_IPHONE
                        if (valueCont > 10) {
                            valueCont = 0;
                            usleep(10000);
                        }else {
                            usleep(1000);
                        }
                        
                        valueCont++;               // iOS device
#elif TARGET_OS_MAC
                            // Other kinds of Mac OS
#else
#   error "Unknown Apple platform"
#endif
                        
                       
                    }else {
#if TARGET_IPHONE_SIMULATOR
                        // iOS Simulator
#elif TARGET_OS_IPHONE
                        if (zeroCount > 3) {
                            zeroCount = 0;
                            usleep(3300);
                        }else {
                            usleep(1000);
                        }
                        zeroCount++;

              // iOS device
#elif TARGET_OS_MAC
                        usleep(3000);
                        // Other kinds of Mac OS
#else
#   error "Unknown Apple platform"
#endif
                        
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
-(NSString*)localAddress{
    if (sess != nil) {
        char *addr =  sess->getLocalIPAddr();
        NSString *addrs = [NSString stringWithUTF8String:addr];
        return addrs;
    }else {
        return @"";
    }
}
-(BOOL)useCell
{
    NSString *ipaddr = [self localAddress];
    NSDictionary *infos = [self address];
    for (NSString *key in infos.allKeys) {
        if ([ipaddr isEqualToString:infos[key]]) {
            if ([key isEqualToString:@"en0"]) {
                return  false;
            }
        }
    }
    return  true;
}
-(NSDictionary *)address
{
    // On iPhone, 3G is "pdp_ipX", where X is usually 0, but may possibly be 0-3 (i'm guessing...)
    
    NSData *result = nil;
    
    struct ifaddrs *addrs;
    const struct ifaddrs *cursor;
    NSMutableDictionary *info = [NSMutableDictionary dictionary];
    if ((getifaddrs(&addrs) == 0))
    {
        cursor = addrs;
        while (cursor != NULL)
        {
            NSLog(@"cursor->ifa_name = %s", cursor->ifa_name);
            
            //            if (strncmp(cursor->ifa_name, "pdp_ip", 6) == 0)
            //            {
            //
            //            }
            if (cursor->ifa_addr->sa_family == AF_INET)
            {
                struct sockaddr_in *addr = (struct sockaddr_in *)cursor->ifa_addr;
                NSLog(@"cursor->ifa_addr = %s", inet_ntoa(addr->sin_addr));
                
                result = [NSData dataWithBytes:addr length:sizeof(struct sockaddr_in)];
                
                NSString *ifname = [NSString stringWithUTF8String:cursor->ifa_name];
                [info setObject:[NSString stringWithUTF8String:inet_ntoa(addr->sin_addr)] forKey:ifname];
                cursor = cursor->ifa_next;
            }
            else
            {
                cursor = cursor->ifa_next;
            }
            
        }
        freeifaddrs(addrs);
    }
    
    return info;
}
@end
