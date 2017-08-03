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
#include "FFmpeg-iOS/include/libavformat/avformat.h"
#include "FFmpeg-iOS/include/libavcodec/avcodec.h"
#include "FFmpeg-iOS/include/libavutil/samplefmt.h"
#include "FFmpeg-iOS/include/libswresample/swresample.h"
#ifdef __cplusplus
}
#endif

namespace PushSDK { namespace ffmpeg {

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

        AVCodec* m_codec;
        AVCodecContext* m_ctx;
        AVFrame* m_frame;
        AVFrame* m_converFrame;
        AVPacket m_pkt;
        SwrContext* m_swrCtx;

        uint8_t* m_samples;
        uint8_t** m_convertData;

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
