//
//  H264Encode.hpp
//  CameraRtmpSDK
//
//  Created by Abson on 16/9/6.
//  Copyright © 2016年 Abson. All rights reserved.
//

#ifndef H264Encode_hpp
#define H264Encode_hpp

#include "IEncoder.hpp"
#ifdef __APPLE__
#include <dispatch/dispatch.h>
#endif

#include <CoreVideo/CoreVideo.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <libavutil/opt.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>

#ifdef __cplusplus
}
#endif

namespace PushSDK { namespace ffmpeg {

    typedef int32_t WORD;

    class H264Encode : public IEncoder
    {
    public:
        H264Encode( int frame_w, int frame_h, int fps, int bitrate);
        ~H264Encode();

    public:
        void setOutput(std::shared_ptr<IOutput> output) {m_output = output;}

        // Input is expecting a CVPixelBufferRef
        void pushBuffer(const uint8_t* const data, size_t size, const IMetadata& metadata);

        void stop();

    public:

        void setBitrate(int bitrate);

        const int bitrate() const { return m_bitrate; }

    public:

        bool setUpWriter();

    private:
        void stopPushEncode();

    private:
        std::weak_ptr<IOutput> m_output;

        int m_bitrate;

        int m_frameW;
        int m_frameH;

        int m_fps;
        int m_frameCount;

        int m_y_size;

        dispatch_queue_t m_queue;

        int m_framecnt;

    private:
        AVFormatContext* m_fmt_ctx;
        AVCodecContext* m_codec_ctx;
        AVStream* m_video_st;
        AVFrame* m_frame;
        AVOutputFormat* m_out_fmt;
        AVCodec* m_codec;
        AVPacket m_pkt;


        uint8_t* m_pic_buf;

//        bool isFirstSli
    };
}
}


#endif /* H264Encode_hpp */
