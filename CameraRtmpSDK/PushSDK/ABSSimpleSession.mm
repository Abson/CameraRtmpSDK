//
//  ABSSimpleSession.m
//  CameraRtmpSDK
//
//  Created by Abson on 16/9/5.
//  Copyright © 2016年 Abson. All rights reserved.
//

#import "ABSSimpleSession.h"
#import "ABSPreviewView.h"
#import "CameraSource.h"
#import "H264Encode.h"
#import "MicSource.h"
#include "AACEncoder.hpp"
#import "opus_encoder.h"

@interface ABSSimpleSession()
{
  dispatch_queue_t _graphManageQueue;
  push_sdk::Apple::CameraSource* cameraSource_;
  push_sdk::Apple::MicSource* micSource_;
  push_sdk::ffmpeg::H264Encode* h264Encoder_;
  std::shared_ptr<push_sdk::ffmpeg::AACEncoder> aacEncoder_;
  std::shared_ptr<push_sdk::ffmpeg::OpusEncoder> opusEncoder_;
}
@property (nonatomic, assign) int bitrate;
@property (nonatomic, assign) CGSize videoSize;
@property (nonatomic, assign) int fps;
@property (nonatomic, assign) BOOL useInterfaceOrientation;
@property (nonatomic, strong) ABSPreviewView* previewView;
@property (nonatomic, assign) ABSCameraState cameraState;
@property (nonatomic, weak) UIView* preview;
@property (nonatomic, assign) float audioSampleRate;
@property (nonatomic, assign) int audioChannelCount;
@end

@implementation ABSSimpleSession
- (void)dealloc
{
  [self endVidoeRecord];
  [self endAudioRecord];

  Cplus_Release(cameraSource_);
  Cplus_Release(h264Encoder_);
  Cplus_Release(micSource_);
}

- (instancetype)init
{
  if (self = [super init]) {


  }
  return self;
}

- (instancetype)initWithVideoSize:(CGSize)videoSize
                              fps:(int)fps
                          bitrate:(int)bps
          useInterfaceOrientation:(BOOL)useInterfaceOrientation
                      cameraState:(ABSCameraState)cameraState
                     previewFrame:(CGRect)previewFrame
{
  if (self = [super init]) {

    self.videoSize = videoSize;
    self.fps = fps;
    self.bitrate = bps;
    self.useInterfaceOrientation = useInterfaceOrientation;
    self.cameraState = cameraState;

    _previewView = [[ABSPreviewView alloc] initWithFrame:CGRectMake(0, 0, previewFrame.size.width, previewFrame.size.height)];

    _graphManageQueue = dispatch_queue_create("com.PushSDK.session.graph", DISPATCH_QUEUE_SERIAL);

    __weak typeof(self) bSelf = self;
    //        dispatch_async(_graphManageQueue, ^{
    [bSelf setupGraph];
    //        });
  }
  return self;
}


- (instancetype)initWithAudioSampleRate:(float)rate
                           channelCount:(int)count;
{
  if (self = [super init]) {
    self.audioSampleRate = rate > 0 ? rate : (float) 44100.; // 最多只能用 44KHZ，因为 flv 格式不支持 44KHZ 以上。
    self.audioChannelCount = count > 0 ? count : 2;

    [self setUpMicSource];
  }
  return self;
}

- (void)setupGraph
{
  cameraSource_ = new push_sdk::Apple::CameraSource();
  cameraSource_->setupCamera(self.fps, (_cameraState == ABSCameraStateFront), true, AVCaptureSessionPreset640x480);
  CALayer* previeLayer = cameraSource_->captureVideoPreviewLayer();
  previeLayer.frame = self.previewView.bounds;
  [self.previewView.layer addSublayer:previeLayer];
}

- (void)setUpMicSource
{
  micSource_ = new push_sdk::Apple::MicSource(self.audioSampleRate, self.audioChannelCount);
}


- (void)startVideoRecord
{
  h264Encoder_ = new push_sdk::ffmpeg::H264Encode(static_cast<unsigned int>(self.videoSize.width),
      static_cast<unsigned int>(self.videoSize.height), self.fps, self.bitrate);
  cameraSource_->setOutput(h264Encoder_);
  cameraSource_->startRecord();
}

- (void)startAudioRecord
{
  NSString* filePath = [self randomOpusPath];

  const char* file_path = [filePath cStringUsingEncoding:NSUTF8StringEncoding];
  opusEncoder_ = std::make_shared<push_sdk::ffmpeg::OpusEncoder>(self.audioSampleRate, self.audioChannelCount, 96000, file_path);
  micSource_->setOutput(opusEncoder_);
  micSource_->start();
}

- (NSString *)randomAACPath {

  NSString *date = nil;
  NSDateFormatter *formatter = [[NSDateFormatter alloc] init];
  formatter.dateFormat = @"YYYY-MM-dd hh:mm:ss";
  date = [formatter stringFromDate:[NSDate date]];
  NSString* file_path = [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, true).lastObject stringByAppendingPathComponent:
      [NSString stringWithFormat:@"%@.aac", date]];

  return file_path;
}

- (NSString *)randomOpusPath {

  NSString *date = nil;
  NSDateFormatter *formatter = [[NSDateFormatter alloc] init];
  formatter.dateFormat = @"YYYY-MM-dd hh:mm:ss";
  date = [formatter stringFromDate:[NSDate date]];
  NSString* file_path = [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, true).lastObject stringByAppendingPathComponent:
      [NSString stringWithFormat:@"%@.opus", date]];

  return file_path;
}

- (void)endAudioRecord
{
  micSource_->stop();
}

- (void)endVidoeRecord
{
  cameraSource_->stopRecord();
}


@end
