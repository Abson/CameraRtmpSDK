//
//  IOutput.hpp
//  CameraRtmpSDK
//
//  Created by Abson on 16/9/6.
//  Copyright © 2016年 Abson. All rights reserved.
//

#ifndef IOutput_hpp
#define IOutput_hpp

#include <chrono>

namespace PushSDK {

    class IOutput
    {
    public:
        virtual void setEpoch(const std::chrono::steady_clock::time_point epoch) {};
        virtual void pushBuffer(const uint8_t* const data, size_t size) = 0;
        virtual void stopPushBuffer() = 0;
        virtual ~IOutput() {};
    };
}


#endif /* IOutput_hpp */
