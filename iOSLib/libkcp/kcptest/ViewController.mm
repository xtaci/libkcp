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

- (void)viewDidLoad {
    
    [super viewDidLoad];
    self.dispatchqueue = dispatch_queue_create("test", NULL);
    [self testCrypto];
    // Do any additional setup after loading the view, typically from a nib.
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
        
        //c.key = [@"1234567890123456789012345678901234567890" dataUsingEncoding:NSUTF8StringEncoding];
        self.tun = [[SFKcpTun alloc] initWithConfig:c ipaddr:self.addr.text port:port queue:self.dispatchqueue];
        self.tun.delegate = self;
    }
    
}
- (IBAction)send:(id)sender {
    
    if ( self.tun == nil ){
        return;
    }
    self.t = [NSTimer scheduledTimerWithTimeInterval:0.5 target:self selector:@selector(sendtest) userInfo:nil repeats:true];
}
- (IBAction)stop:(id)sender {
    [self.t invalidate];
}
-(void)sendtest
{
    for (int i = 0; i < 1000; i++) {
        char  *ptr = (char  *)BlockCrypt::ramdonBytes(40960);
        NSData *d = [NSData dataWithBytes:(void*)ptr length:40960];
        free(ptr);
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
    NSLog(@"recv: %lu",(unsigned long)data.length);
}
- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}


@end
