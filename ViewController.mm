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
#import "H264Encode.h"

@interface ViewController ()<ABSSessionDelegate, AVCaptureVideoDataOutputSampleBufferDelegate>

@property (nonatomic, strong) ABSSimpleSession* session;

//{
//    AVCaptureSession                *captureSession;
//    AVCaptureDevice                 *captureDevice;
//    AVCaptureDeviceInput            *captureDeviceInput;
//    AVCaptureVideoDataOutput        *captureVideoDataOutput;
//
//    AVCaptureConnection             *videoCaptureConnection;
//
//    AVCaptureVideoPreviewLayer      *previewLayer;
//    PushSDK::ffmpeg::H264Encode* encoder;
//}

@end

@implementation ViewController

- (void)viewDidLoad {
    [super viewDidLoad];

    self.session = [[ABSSimpleSession alloc] initWithVideoSize:CGSizeMake(640, 480) fps:30 bitrate:1000000 useInterfaceOrientation:true cameraState:ABSCameraStateFront previewFrame:self.view.bounds];
    self.session.delegate = self;
    [self.view addSubview:self.session.previewView];

    /*
    captureSession = [[AVCaptureSession alloc] init];
    //    captureSession.sessionPreset = AVCaptureSessionPresetHigh;
    if ([captureSession canSetSessionPreset:AVCaptureSessionPreset640x480]) {
        captureSession.sessionPreset = AVCaptureSessionPreset640x480;
    }


    // captureDevice = [AVCaptureDevice defaultDeviceWithMediaType:AVMediaTypeVideo];

    int position =  AVCaptureDevicePositionFront;
        
    NSArray* devices = [AVCaptureDevice devices];
    for (AVCaptureDevice* device in devices) {
        if ([device hasMediaType:AVMediaTypeVideo] && device.position == position ) {
            captureDevice= device;
        }
    }

    NSError *error = nil;
    captureDeviceInput = [AVCaptureDeviceInput deviceInputWithDevice:captureDevice error:&error];

    if([captureSession canAddInput:captureDeviceInput])
        [captureSession addInput:captureDeviceInput];
    else
        NSLog(@"Error: %@", error);

    dispatch_queue_t queue = dispatch_queue_create("myEncoderH264Queue", NULL);

    captureVideoDataOutput = [[AVCaptureVideoDataOutput alloc] init];
    [captureVideoDataOutput setSampleBufferDelegate:self queue:queue];
    NSDictionary *settings = [[NSDictionary alloc] initWithObjectsAndKeys:
                              [NSNumber numberWithUnsignedInt:kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange],
                              kCVPixelBufferPixelFormatTypeKey,
                              nil]; // X264_CSP_NV12
    captureVideoDataOutput.videoSettings = settings;
    captureVideoDataOutput.alwaysDiscardsLateVideoFrames = YES;

    if ([captureSession canAddOutput:captureVideoDataOutput]) {
        [captureSession addOutput:captureVideoDataOutput];
    }

    // 保存Connection，用于在SampleBufferDelegate中判断数据来源（是Video/Audio？）
    videoCaptureConnection = [captureVideoDataOutput connectionWithMediaType:AVMediaTypeVideo];

#pragma mark -- AVCaptureVideoPreviewLayer init
    previewLayer = [AVCaptureVideoPreviewLayer layerWithSession:captureSession];
    previewLayer.frame = self.view.layer.bounds;
    previewLayer.videoGravity = AVLayerVideoGravityResizeAspectFill; // 设置预览时的视频缩放方式
    [[previewLayer connection] setVideoOrientation:AVCaptureVideoOrientationPortrait]; // 设置视频的朝向
    [self.view.layer addSublayer:previewLayer];
     */
    UIButton* record = [[UIButton alloc] initWithFrame:CGRectMake(0, 0, 80, 50)];
    record.center = CGPointMake(self.view.frame.size.width * 0.5, self.view.frame.size.height - 75);
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
//        encoder = new PushSDK::ffmpeg::H264Encode(640, 480, 30,1000000);
//        [captureSession startRunning];
        [self.session startRecord];
    }
    else {
        [self.session endRecord];
//        [captureSession stopRunning];
//        encoder->stopPushEncode();
    }
}

//- (void)captureOutput:(AVCaptureOutput *)captureOutput didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer fromConnection:(AVCaptureConnection *)connection
//{
//
//    // 这里的sampleBuffer就是采集到的数据了，但它是Video还是Audio的数据，得根据connection来判断
//    if (connection == videoCaptureConnection) {
//        CVPixelBufferRef imageBuffer = CMSampleBufferGetImageBuffer(sampleBuffer);
//        // Video
//        //        NSLog(@"在这里获得video sampleBuffer，做进一步处理（编码H.264）");
//         if (CVPixelBufferLockBaseAddress(imageBuffer, 0) == kCVReturnSuccess) {
//             encoder->encodeBuffer(imageBuffer);
//         }
//
//        CVPixelBufferUnlockBaseAddress(imageBuffer, 0);
//    }
//    //    else if (connection == _audioConnection) {
//    //
//    //        // Audio
//    //        NSLog(@"这里获得audio sampleBuffer，做进一步处理（编码AAC）");
//    //    }
//    
//}


- (void)didAddCamearSource:(ABSSimpleSession *)session
{
    
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];

}

@end
