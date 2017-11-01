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

@interface ABSSimpleSession()
{
    dispatch_queue_t _graphManageQueue;
    PushSDK::Apple::CameraSource* m_cameraSource;
    PushSDK::Apple::MicSource* m_micSource;
    PushSDK::ffmpeg::H264Encode* m_h264Encoder;
    std::shared_ptr<PushSDK::ffmpeg::AACEncoder> m_aacEncoder;
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

    Cplus_Release(m_cameraSource);
    Cplus_Release(m_h264Encoder);
    Cplus_Release(m_micSource);
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
                                    fps:(int)fps
                                bitrate:(int)bps
                           channelCount:(int)count
{
    if (self = [super init]) {
        self.audioSampleRate = rate > 0 ? rate : (float) 44100.; // 最多只能用 44KHZ，因为 flv 格式不支持 44KHZ 以上。
        self.fps = fps;
        self.bitrate = bps;
        self.audioChannelCount = count > 0 ? count : 2;

        [self setUpMicSource];
    }
    return self;
}

- (void)setupGraph
{
    m_cameraSource = new PushSDK::Apple::CameraSource();
    m_cameraSource->setupCamera(self.fps, (_cameraState == ABSCameraStateFront), true, AVCaptureSessionPreset640x480);
    CALayer* previeLayer = m_cameraSource->captureVideoPreviewLayer();
    previeLayer.frame = self.previewView.bounds;
    [self.previewView.layer addSublayer:previeLayer];
}

- (void)setUpMicSource
{
    m_micSource = new PushSDK::Apple::MicSource(self.audioSampleRate, self.audioChannelCount);
}


- (void)startVideoRecord
{
    m_h264Encoder = new PushSDK::ffmpeg::H264Encode(self.videoSize.width, self.videoSize.height, self.fps, self.bitrate);
    m_cameraSource->setOutput(m_h264Encoder);
    m_cameraSource->startRecord();
}

- (void)startAudioRecord
{
    NSString *date = nil;
    NSDateFormatter *formatter = [[NSDateFormatter alloc] init];
    formatter.dateFormat = @"YYYY-MM-dd hh:mm:ss";
    date = [formatter stringFromDate:[NSDate date]];
    NSString* file_path = [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, true).lastObject stringByAppendingPathComponent:[NSString stringWithFormat:@"%@.aac", date]];
    m_aacEncoder = std::shared_ptr<PushSDK::ffmpeg::AACEncoder>(new PushSDK::ffmpeg::AACEncoder((int) self.audioSampleRate, self.audioChannelCount, 96000, [file_path cStringUsingEncoding:NSUTF8StringEncoding]));
    m_micSource->setOutput(m_aacEncoder);
    m_micSource->start();
}

- (void)endAudioRecord
{
     m_micSource->stop();
}

- (void)endVidoeRecord
{
     m_cameraSource->stopRecord();
}


@end
