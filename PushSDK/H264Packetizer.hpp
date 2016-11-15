//
//  H264Packetizer.hpp
//  CameraRtmpSDK
//
//  Created by Abson on 16/9/6.
//  Copyright © 2016年 Abson. All rights reserved.
//

#ifndef H264Packetizer_hpp
#define H264Packetizer_hpp

#include <stdio.h>
#include "ITransform.hpp"
#include <vector>

namespace PushSDK { namespace rtmp {

    class H264Packetizer : public ITransform
    {
    public:
        H264Packetizer(int ctsOffset = 0);

    public:
        void pushBuffer(const uint8_t* const data, size_t size);

        void stopPushBuffer() {}

        void setOutput(IOutput* output);
        void setEpoch(const std::chrono::steady_clock::time_point epoch) { m_epoch = epoch; };
        

    private:
        std::chrono::steady_clock::time_point m_epoch;
        IOutput* m_output;
        std::vector<uint8_t> m_sps;
        std::vector<uint8_t> m_pps;
        std::vector<uint8_t> m_outbuffer;

        double m_videoTs;

        std::vector<uint8_t> configurationFromSpsAndPps();

        int m_ctsOffset;

        bool m_sentConfig;
    };
}
}


#endif /* H264Packetizer_hpp */
