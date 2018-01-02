//
//  ITransform.hpp
//  CameraRtmpSDK
//
//  Created by Abson on 16/9/6.
//  Copyright © 2016年 Abson. All rights reserved.
//

#ifndef ITransform_hpp
#define ITransform_hpp

#include "IOutput.hpp"

namespace push_sdk {

    class ITransform: public IOutput
    {
    public:
        virtual void setOutput(std::shared_ptr<IOutput>output) = 0;
        
        virtual ~ITransform() {};
    };
}

#endif /* ITransform_hpp */
