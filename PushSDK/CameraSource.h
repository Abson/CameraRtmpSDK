//
//  CameeaSource.h
//  CameraRtmpSDK
//
//  Created by Abson on 16/9/14.
//  Copyright © 2016年 Abson. All rights reserved.
//

#ifndef __Apple_CameraSource_h
#define __Apple_CameraSource_h

#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>
#import "IOutput.hpp"

namespace PushSDK {  namespace Apple {
	class CameraSource
	{
	public:
		CameraSource();
		~CameraSource();
		
		void setupCamera(int fps, bool  useFront = false, bool useInterfaceOrientation = false, NSString* sessionPreset = nil);

        void startRecord();
        void stopRecord();

        void captureBuffer(CVPixelBufferRef pb);

        void setOutput(IOutput* output);

        AVCaptureConnection* outputConnection();

        AVCaptureVideoPreviewLayer* captureVideoPreviewLayer();
    private:
        void* m_captureSession;
        void* m_captureDevice;
        void* m_input;
        void* m_captureVideoDataOutput;
        void* m_callbackSession;
        void* m_outputConnection;
        void* m_captureVideoPreviewLayer;
        IOutput* m_output;
        dispatch_queue_t m_camera_queue;
	};
}
}

#endif






