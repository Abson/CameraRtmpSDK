//
//  AACEncoder.hpp
//  CameraRtmpSDK
//
//  Created by Abson on 2016/11/23.
//  Copyright © 2016年 Abson. All rights reserved.
//

#ifndef AACEncoder_hpp
#define AACEncoder_hpp

#include "IEncoder.hpp"

//struct AVCodec;
//struct AVCodecContext;
//struct AVFrame;
//struct AVPacket;

#ifdef __cplusplus
extern "C" {
#endif
#include "avformat.h"
#include "avcodec.h"
#include "samplefmt.h"
#include "swresample.h"
#ifdef __cplusplus
}
#endif

namespace push_sdk { namespace ffmpeg {

    class AACEncoder : public IEncoder {
        
    public:

        AACEncoder(int frequencyInHz, int channelCount, int bitrate, const std::string& filename = "");

        ~AACEncoder();

        void pushBuffer(const uint8_t* const data, size_t size, const IMetadata& metadata);

        void setOutput(std::shared_ptr<IOutput> output);

        void setBitrate(int bitrate) {  m_birerate = bitrate; }

        const int bitrate() const { return m_birerate; }

        void stop();

    private:
        void initAudioResample();
        void audioResample(const AVFrame& coverFrame);
        
        AVFrame*
        create_av_frame( const int nb_samples, enum AVSampleFormat sample_fmt, uint64_t channel_layout, int sample_rate);
    private:

        std::weak_ptr<IOutput> m_output;

        int m_birerate;

        AVCodec* codec_;
        AVCodecContext* code_ctx_;
        AVFrame* frame_;
        AVFrame* conver_frame_;
        AVPacket pkt_;
        SwrContext* swr_ctx_;

        uint8_t* samples_;
        uint8_t** convert_data_;

        int m_framecnt;

    private: // 测试专用
        FILE* m_file;

        AVFormatContext* m_fmtCtx;
        AVStream* m_stream;
        AVOutputFormat* m_outfmt;
    };
}
}




#endif /* AACEncoder_hpp */
