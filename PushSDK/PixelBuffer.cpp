//
//  PixelBuffer.cpp
//  CameraRtmpSDK
//
//  Created by Abson on 16/9/6.
//  Copyright © 2016年 Abson. All rights reserved.
//

#include "PixelBuffer.hpp"


namespace PushSDK { namespace Apple {

    PixelBuffer:: PixelBuffer(CVPixelBufferRef pb, bool temporary)
    :m_state(kVCPixelBufferStateAvailable),
    m_locked(false),
    m_pixelBuffer(CVPixelBufferRetain(pb)),
    m_temporary(temporary)
    {
        m_pixelFormat = (PixelBufferFormatType)CVPixelBufferGetPixelFormatType(pb);
    }

    PixelBuffer::~PixelBuffer()
    {
        CVPixelBufferRelease(m_pixelBuffer);
    }

    void
    PixelBuffer::lock(bool readOnly)
    {
        m_locked = true;
    }

    void
    PixelBuffer::unlock(bool readOnly)
    {
        m_locked = false;
    }
}
}