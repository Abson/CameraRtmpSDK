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
#include "IMetaData.hpp"

namespace push_sdk {

    class IOutput
    {
    public:
        virtual void set_epoch(const std::chrono::steady_clock::time_point epoch) = 0;
        virtual void PushBuffer(const uint8_t *const data, size_t size, const IMetadata &metadata = IMetadata()) = 0;
        virtual void stop() = 0;
        virtual ~IOutput() {};
    };
}


#endif /* IOutput_hpp */
