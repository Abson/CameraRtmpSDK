//
//  H264Parser.hpp
//  CameraRtmpSDK
//
//  Created by Abson on 2016/11/14.
//  Copyright © 2016年 Abson. All rights reserved.
//

#ifndef H264Parser_hpp
#define H264Parser_hpp

#include <stdio.h>
#include <iostream>

struct AVPacket;


namespace VideoCore { namespace Parser {

    class H264Parser {

    public:
        static void simplest_h264_parser(uint8_t* buf, int len);
    };
}
}


#endif /* H264Parser_hpp */
