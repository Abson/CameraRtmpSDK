//
//  PixelBuffer.hpp
//  CameraRtmpSDK
//
//  Created by Abson on 16/9/6.
//  Copyright © 2016年 Abson. All rights reserved.
//

#ifndef PixelBuffer_hpp
#define PixelBuffer_hpp

#include "IPixleBuffer.hpp"
#include <CoreMedia/CoreMedia.h>
#include <memory>

namespace PushSDK { namespace Apple {

    class PixelBuffer : public IPixleBuffer
    {
    public:
        PixelBuffer(CVPixelBufferRef pb, bool temporary = false);
        ~PixelBuffer();

        static const size_t hash(PixelBuffer* buf) {
            return std::hash<PixelBuffer*>()(buf);
        };

    public:
        const int width() const {return (int)CVPixelBufferGetWidth(m_pixelBuffer);}
        const int height() const {return (int)CVPixelBufferGetHeight(m_pixelBuffer);}
        const void* baseAddress() const {return CVPixelBufferGetBaseAddress(m_pixelBuffer);};

        const PixelBufferFormatType pixelFormat() const {return m_pixelFormat;}

        void lock(bool readOnly = false);
        void unlock(bool readOnly = false);

        void setState(const PixelBufferState state) {m_state = state;}
        const PixelBufferState state() const {return m_state;}

        const bool isTemporary() const {return m_temporary;}
        void setTemporary(const bool temporary) {m_temporary = temporary;}

    public:
        const CVPixelBufferRef cvBuffer() const {return  m_pixelBuffer;}

    private:
        CVPixelBufferRef m_pixelBuffer;
        PixelBufferFormatType m_pixelFormat;
        PixelBufferState m_state;

        bool m_locked;
        bool m_temporary;
    };
}
}


#endif /* PixelBuffer_hpp */
