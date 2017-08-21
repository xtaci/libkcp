//
//  ViewController.m
//  kcptest
//
//  Created by 孔祥波 on 28/04/2017.
//  Copyright © 2017 Kong XiangBo. All rights reserved.
//

#import "ViewController.h"
#import "BlockCrypt.h"
#import "Crypt.h"
@interface ViewController ()
@property (weak, nonatomic) IBOutlet UITextField *addr;
@property (weak, nonatomic) IBOutlet UITextField *port;
@property (strong,nonatomic) SFKcpTun *tun;
@property (strong,nonatomic) dispatch_queue_t dispatchqueue;
@property (nonatomic,strong) NSTimer *t;

@end

@implementation ViewController
{
    NSDate *last;
    dispatch_source_t dispatchSource;
    dispatch_queue_t tqueue ;
    dispatch_queue_t socketqueue ;
    
}
- (void)viewDidLoad {
    
    [super viewDidLoad];
//    self.dispatchqueue = dispatch_queue_create("test", NULL);
//    [self testCrypto];
//    [self testSodium];
    tqueue  = dispatch_queue_create("test.yarshure", DISPATCH_QUEUE_SERIAL);
    // Do any additional setup after loading the view, typically from a nib.
}
-(IBAction)tThread:(id)sender{
    last = [NSDate date];
    [NSThread detachNewThreadWithBlock:^{
        for (; ; ) {
            NSDate *n = [NSDate date];
            //NSLog(@"timer come %0.6f",[n timeIntervalSinceDate:last]);
            last = n;
            [NSThread sleepForTimeInterval:0.003];
        }
    }];
}
-(IBAction)testTimer:(id)sender{
    
    // Create a dispatch source that'll act as a timer on the concurrent queue
    // You'll need to store this somewhere so you can suspend and remove it later on
    NSLog(@"go");
    dispatchSource = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0,tqueue);
    last = [NSDate date];
    // Setup params for creation of a recurring timer
    double interval = 3.0;
    dispatch_time_t startTime = dispatch_time(DISPATCH_TIME_NOW, 0);
    uint64_t intervalTime = (int64_t)(interval * NSEC_PER_MSEC);
    dispatch_source_set_timer(dispatchSource, startTime, intervalTime, 0);
    
    // Attach the block you want to run on the timer fire
    dispatch_source_set_event_handler(dispatchSource, ^{
        // Your code here
        NSDate *n = [NSDate date];
        //NSLog(@"timer come %0.6f",[n timeIntervalSinceDate:last]);
        last = n;
    });
    
    
    dispatch_resume(dispatchSource);
    
    
    
    
}
-(void)testCrypto2
{
    
}
- (void)testSodium
{
    //sodium_init();
    //NSData *s = [@"0123456789ABCDEF0123456789ABCDEF" dataUsingEncoding:NSUTF8StringEncoding];
}
- (void)testCrypto{
    NSData *s = [@"0123456789ABCDEF0123456789ABCDEF" dataUsingEncoding:NSUTF8StringEncoding];
    NSLog(@"org %@",s);
    Crypt *block = [[Crypt alloc] initWithKey:s crypto:@"aes"];  //blockWith(s.bytes, "aes");
    size_t outlen = 0;
    [block encrypt:s] ;//encrypt(s.bytes, 32, &outlen);
    //NSData *outData = [NSData dataWithBytes:outbuffer length:outlen];
    NSLog(@"en %@",s);
    
    [block decrypt:s];
    NSLog(@"de %@",s);
    [self testB];
    
}
-(void)testB{
    NSData *s = [@"0123456789ABCDEF0123456789ABCDEF" dataUsingEncoding:NSUTF8StringEncoding];
    NSLog(@"org %@",s);
    BlockCrypt *block =  BlockCrypt::blockWith(s.bytes, "aes"); //blockWith(s.bytes, "aes");
    size_t outlen = 0;
    block->encrypt((void *)s.bytes, 32, &outlen);
    NSData *outData = [NSData dataWithBytes:s.bytes  length:outlen];
     NSLog(@"en %@",s);
    
    block->decrypt((void*)s.bytes, s.length, &outlen);
   
    NSLog(@"en %@",s);
    free(block);

}
- (IBAction)go:(id)sender {
    //kcptest(, );
    const char *addr = [self.addr.text UTF8String];
    NSInteger port = [self.port.text integerValue];
    if (self.tun == nil) {
        TunConfig *c = [[TunConfig alloc] init];
        c.dataShards = 2;
        c.parityShards = 2;
        c.iptos = 46;
        c.crypt = @"aes";
        c.key = [@"1234567890123456789012345678901234567890" dataUsingEncoding:NSUTF8StringEncoding];
        self.tun = [[SFKcpTun alloc] initWithConfig:c ipaddr:self.addr.text port:port queue:self.dispatchqueue];
        self.tun.delegate = self;
    }
    
}
- (IBAction)send:(id)sender {
    
    if ( self.tun == nil ){
        return;
    }
    [self sendtest];
    //self.t = [NSTimer scheduledTimerWithTimeInterval:0.5 target:self selector:@selector(sendtest) userInfo:nil repeats:true];
}
- (IBAction)stop:(id)sender {
    [self.t invalidate];
}
-(void)sendtest
{
    for (int i = 0; i < 2; i++) {
        NSString *msg = [NSString stringWithFormat:@"message %d",i];
        NSData *d = [msg dataUsingEncoding:NSUTF8StringEncoding];
        //char  *ptr = (char  *)BlockCrypt::ramdonBytes(40960);
        //NSData *d = [NSData dataWithBytes:(void*)ptr length:40960];
        //free(ptr);
        [self.tun input:d];
    }
}
-(IBAction)shutdown:(id)sender
{
    if (self.tun != nil){
        [self.tun shutdownUDPSession];
        self.tun = nil;
    }
}
-(void)didRecevied:(NSData*)data{
    NSLog(@"recv: %@",data);
}
- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}


@end
