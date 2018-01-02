//
//  ViewController.m
//  CameraRtmpSDK
//
//  Created by Abson on 16/9/2.
//  Copyright © 2016年 Abson. All rights reserved.
//

#import "ViewController.h"
#import "ABSSimpleSession.h"
#import "ABSPreviewView.h"
#import <AVFoundation/AVFoundation.h>
#import <iostream>
//#import "H264Encode.h"
#import "opus_encoder.h"
#include "audio_mixer.h"

//#define kTestAudioMixer 1 // a switch to open audio mixer

@interface ViewController ()<ABSSessionDelegate, AVCaptureVideoDataOutputSampleBufferDelegate>
{
  push_sdk::ffmpeg::OpusEncoder* encoder_;
}
@property (nonatomic, strong) ABSSimpleSession* session;

@end

@implementation ViewController

- (void)viewDidLoad {
  [super viewDidLoad];

  self.session = [[ABSSimpleSession alloc] initWithAudioSampleRate:16000 channelCount:2];
  self.session.delegate = self;

  UIButton* record = [[UIButton alloc] initWithFrame:CGRectMake(0, 0, 80, 50)];
  record.center = CGPointMake(self.view.frame.size.width * 0.5f, self.view.frame.size.height - 75);
  [record setBackgroundColor:[UIColor greenColor]];
  [record setTitle:@"录制" forState:UIControlStateNormal];
  [record setTitle:@"取消录制" forState:UIControlStateSelected];
  [record addTarget:self action:@selector(record:) forControlEvents:UIControlEventTouchUpInside];
  [self.view addSubview:record];
}

#pragma mark - ABSSessionDelegate

- (void)record:(UIButton*)sender
{
  sender.selected = !sender.isSelected;
  [sender setBackgroundColor: sender.isSelected ? [UIColor redColor] : [UIColor greenColor]];

  if (sender.isSelected) {
    [self.session startAudioRecord];
  } else {
    [self.session endAudioRecord];
  }

//  NSString* path1 = [[NSBundle mainBundle] pathForResource:@"tdjm.pcm" ofType:nil];
//  const char* input1 = [path1 cStringUsingEncoding:NSUTF8StringEncoding];
//  encoder_->Test(input1);
}

- (void)touchesBegan:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event
{
#ifdef kTestAudioMixer
  @autoreleasepool {
    NSString* path1 = [[NSBundle mainBundle] pathForResource:@"audio1.wav" ofType:nil];
    NSString* path2 = [[NSBundle mainBundle] pathForResource:@"audio2.wav" ofType:nil];
    NSString* path3 = [[NSBundle mainBundle] pathForResource:@"audio3.wav" ofType:nil];
    NSString* path4 = [[NSBundle mainBundle] pathForResource:@"audio4.wav" ofType:nil];
    NSString* outputPath = [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, true).lastObject stringByAppendingPathComponent:@"audio127.wav"];
    
    const char* input1 = [path1 cStringUsingEncoding:NSUTF8StringEncoding];
    const char* input2 = [path2 cStringUsingEncoding:NSUTF8StringEncoding];
    const char* input3 = [path3 cStringUsingEncoding:NSUTF8StringEncoding];
    const char* input4 = [path4 cStringUsingEncoding:NSUTF8StringEncoding];
    const char* output = [outputPath cStringUsingEncoding:NSUTF8StringEncoding];
    
    std::vector<std::string> inputs{std::string(input1)};
    
    clock_t start_time = clock();
    push_sdk::ffmpeg::audio_mixer mixer(inputs, output);
    mixer.StartMixAudio();
    clock_t end_time= clock();
    std::cout<< "Running time is: "<<static_cast<double>(end_time-start_time)/CLOCKS_PER_SEC*1000<<"ms"<< std::endl;//
  }
#endif
}

/* Encode pcm file to opus format which in order to test api*/
- (void)p_testEncodeOpusApi  {

  NSString* outputPath = [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, true).lastObject stringByAppendingPathComponent:@"audio127.opus"];
  const char* output = [outputPath cStringUsingEncoding:NSUTF8StringEncoding];
  encoder_ = new push_sdk::ffmpeg::OpusEncoder(44100, 2, 64000, output);
}

- (void)didReceiveMemoryWarning {
  [super didReceiveMemoryWarning];

}

@end
