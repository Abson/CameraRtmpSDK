//
//  CameeaSource.m
//  CameraRtmpSDK
//
//  Created by Abson on 16/9/14.
//  Copyright © 2016年 Abson. All rights reserved.
//

#include "CameraSource.h"

@interface CaptureCallbackSession : NSObject <AVCaptureVideoDataOutputSampleBufferDelegate>
{
  push_sdk::Apple::CameraSource* m_source;
}

@end

@implementation CaptureCallbackSession

- (void)setSource:(push_sdk::Apple::CameraSource*)source
{
  m_source = source;
}

- (void)captureOutput:(AVCaptureOutput *)captureOutput
didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer
       fromConnection:(AVCaptureConnection *)connection
{

  if (m_source && m_source->outputConnection() == connection) {
    m_source->captureBuffer(CMSampleBufferGetImageBuffer(sampleBuffer));
  }
}

@end


namespace push_sdk {  namespace Apple {

    CameraSource::CameraSource() :
        m_captureSession(nullptr),
        m_captureDevice(nullptr),
        m_input(nullptr),
        m_captureVideoDataOutput(nullptr),
        m_callbackSession(nullptr),
        m_outputConnection(nullptr),
        m_captureVideoPreviewLayer(nullptr),
        m_output(nullptr),
        m_camera_queue(dispatch_queue_create("com.PushSDK.Apple.Camera", nullptr))
    {
    }

    CameraSource::~CameraSource()
    {
      m_captureSession = nil;
      m_captureDevice = nil;
      m_input = nil;
      m_captureVideoDataOutput = nil;
      m_callbackSession = nil;
      m_outputConnection = nil;
      m_captureVideoPreviewLayer = nil;
      m_output = nil;
    }

    void
    CameraSource::setupCamera(int fps, bool  useFront , bool useInterfaceOrientation , NSString* sessionPreset)
    {
      AVCaptureSession* captureSession = [[AVCaptureSession alloc] init];
      if (sessionPreset && [captureSession canSetSessionPreset:sessionPreset]) {
        [captureSession setSessionPreset:sessionPreset];
      }
      else if ([captureSession canSetSessionPreset:AVCaptureSessionPreset640x480]){
        [captureSession setSessionPreset:AVCaptureSessionPreset640x480];
      }
      this->m_captureSession = (__bridge void*)captureSession;

      // intput
      AVCaptureDevicePosition position = useFront ? AVCaptureDevicePositionFront : AVCaptureDevicePositionBack;
      NSArray* devices = [AVCaptureDevice devices];
      for (AVCaptureDevice* device in devices) {
        if ([device hasMediaType:AVMediaTypeVideo] && [device position] == position) {
          this->m_captureDevice = (__bridge void *) device;
        }
      }
      NSError* error = nil;
      AVCaptureDeviceInput*input = [AVCaptureDeviceInput deviceInputWithDevice:(__bridge AVCaptureDevice *)this->m_captureDevice error:&error];
      this->m_input = (__bridge void*)input;
      if ( [(__bridge AVCaptureSession*)this->m_captureSession canAddInput:input]) {
        [(__bridge AVCaptureSession*)this->m_captureSession addInput:input];
      }

      // output
      if (!this->m_callbackSession) {
        CaptureCallbackSession* callbackSession = [[CaptureCallbackSession alloc] init];
        [callbackSession setSource:this];
        this->m_callbackSession = (__bridge_retained void*)callbackSession;
      }
      NSDictionary* videoSet = @{
          (NSString*)kCVPixelBufferPixelFormatTypeKey : @(kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange) //YUV格式, 转出来是 NV12 格式
      };
      AVCaptureVideoDataOutput* captureVideoDataOutput = [[AVCaptureVideoDataOutput alloc] init];
      [captureVideoDataOutput setSampleBufferDelegate:(__bridge CaptureCallbackSession*)this->m_callbackSession queue:m_camera_queue];
      [captureVideoDataOutput setVideoSettings:videoSet];
      captureVideoDataOutput.alwaysDiscardsLateVideoFrames = YES;
      this->m_captureVideoDataOutput = (__bridge void*)captureVideoDataOutput;
      if ([(__bridge AVCaptureSession*)this->m_captureSession canAddOutput:captureVideoDataOutput]) {
        [(__bridge AVCaptureSession*)this->m_captureSession addOutput:captureVideoDataOutput];
      }

      AVCaptureConnection* connection = [captureVideoDataOutput connectionWithMediaType:AVMediaTypeVideo];
      m_outputConnection = (__bridge void*)connection;

      // m_captureVideoPreviewLayer
      AVCaptureVideoPreviewLayer* videoPrevidewLayer = [AVCaptureVideoPreviewLayer layerWithSession:captureSession];
      [videoPrevidewLayer setVideoGravity:AVLayerVideoGravityResizeAspectFill];
      this->m_captureVideoPreviewLayer = (__bridge void*)videoPrevidewLayer;
    }

    void
    CameraSource::startRecord()
    {
      [(__bridge AVCaptureSession*)this->m_captureSession startRunning];
    }

    void
    CameraSource::stopRecord()
    {
      if (m_captureSession) {
        [(__bridge AVCaptureSession*)m_captureSession stopRunning];
        m_captureSession = nil;
      }

      // 停止推流了，关闭所有资源
      dispatch_async(m_camera_queue, ^{
        m_output->stop();
      });

      if (m_callbackSession)
      {
        m_callbackSession = nil;
      }
    }

    void
    CameraSource::captureBuffer(CVPixelBufferRef pb)
    {
      if (!m_output  ) { return;}
      if (CVPixelBufferLockBaseAddress(pb, 0) == kCVReturnSuccess) {

        m_output->pushBuffer((uint8_t*)pb, CVPixelBufferGetDataSize(pb));
      }
      CVPixelBufferUnlockBaseAddress(pb, 0);
    }

    void
    CameraSource::setOutput(push_sdk::IOutput *output)
    {
      m_output = output;
    }

    AVCaptureConnection *
    CameraSource::outputConnection()
    {
      return (__bridge AVCaptureConnection*)m_outputConnection;
    }

    AVCaptureVideoPreviewLayer*
    CameraSource::captureVideoPreviewLayer()
    {
      return (__bridge AVCaptureVideoPreviewLayer*)m_captureVideoPreviewLayer;
    }
  }
}





















