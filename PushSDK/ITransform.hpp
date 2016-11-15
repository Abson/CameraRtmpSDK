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

namespace PushSDK {

    class ITransform: public IOutput
    {
    public:
        virtual void setOutput(IOutput* output) = 0;
        
        virtual ~ITransform() {};
    };
}

#endif /* ITransform_hpp */
