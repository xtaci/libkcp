//
//  ViewController.m
//  kcptest
//
//  Created by 孔祥波 on 28/04/2017.
//  Copyright © 2017 Kong XiangBo. All rights reserved.
//

#import "ViewController.h"

@interface ViewController ()
@property (weak, nonatomic) IBOutlet UITextField *addr;
@property (weak, nonatomic) IBOutlet UITextField *port;
@property (strong,nonatomic) SFKcpTun *tun;
@end

@implementation ViewController

- (void)viewDidLoad {
    
    [super viewDidLoad];
    // Do any additional setup after loading the view, typically from a nib.
}
- (IBAction)go:(id)sender {
    //kcptest(, );
    const char *addr = [self.addr.text UTF8String];
    int port = [self.port.text integerValue];
    if (self.tun == nil) {
        TunConfig *c = [[TunConfig alloc] init];
        self.tun = [[SFKcpTun alloc] initWithConfig:c ipaddr:self.addr.text port:port];
        self.tun.delegate = self;
    }
    
}
- (IBAction)send:(id)sender {
    
    if ( self.tun == nil ){
        return;
    }
    for (int i = 0; i < 10; i++) {
        NSString *msg = [NSString stringWithFormat:@"message %d",i];
        NSData *d = [msg dataUsingEncoding:NSUTF8StringEncoding];
        [self.tun input:d];
    }
}
-(void)didRecevied:(NSData*)data{
    NSLog(@"recv %@",data);
}
- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}


@end
