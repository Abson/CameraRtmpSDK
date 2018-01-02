//
//  IPixleBuffer.hpp
//  CameraRtmpSDK
//
//  Created by Abson on 16/9/6.
//  Copyright © 2016年 Abson. All rights reserved.
//

#ifndef IPixleBuffer_hpp
#define IPixleBuffer_hpp

#include <stdio.h>
#include <stdint.h>

namespace push_sdk {

    enum {
        kVCPixelBufferFormat32BGRA = 'bgra',
        kVCPixelBufferFormat32RGBA = 'rgba',
        kVCPixelBufferFormatL565 = 'L565',
        kCVPixelBufferFormat420v = '420v',
    } PixelBufferFormatType_;

    typedef  uint32_t PixelBufferFormatType;

    typedef enum {
        kVCPixelBufferStateAvailable,
        kVCPixelBufferStateDequeued,
        kVCPixelBufferStateEnqueued,
        kVCPixelBufferStateAcquired
    } PixelBufferState;

    class IPixleBuffer {
    public:
        virtual ~IPixleBuffer() {};
        virtual const int width() const = 0;
        virtual const int height() const = 0;

        virtual const PixelBufferFormatType pixelFormat() const = 0;

        virtual const void* baseAddress() const = 0;

        virtual void lock(bool readOnly = false) = 0;
        virtual void unlock(bool readOnly = false) = 0;

        virtual void setState(const PixelBufferState state) = 0;
        virtual const PixelBufferState state() const = 0;

        virtual const bool isTemporary() const = 0; /* mark if the pixel buffer needs to disappear soon */
        virtual void setTemporary(const bool temporary) = 0;
    };
}

#endif /* IPixleBuffer_hpp */
