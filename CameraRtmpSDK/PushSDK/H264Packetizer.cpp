//
//  H264Packetizer.cpp
//  CameraRtmpSDK
//
//  Created by Abson on 16/9/6.
//  Copyright © 2016年 Abson. All rights reserved.
//

#include "H264Packetizer.hpp"


namespace push_sdk { namespace rtmp {

    H264Packetizer::H264Packetizer(int ctsOffset)
    :m_videoTs(0),
    m_sentConfig(false),
    m_ctsOffset(ctsOffset)
    {
    }

    void
    H264Packetizer::setOutput(IOutput* output)
    {
        m_output = output;
    }

    void
    H264Packetizer::pushBuffer(const uint8_t *const inBuffer, size_t size, const IMetadata& metadata)
    {
        std::vector<uint8_t>& outBuffer = m_outbuffer;
        outBuffer.clear();

        
    }
}
}
