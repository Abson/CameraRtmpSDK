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

namespace push_sdk { namespace ffmpeg {

    typedef int32_t WORD;

    class H264Encode : public IEncoder
    {
    public:
      H264Encode( unsigned frame_w, unsigned frame_h, int fps, int bitrate);
      ~H264Encode();

    public:
      void set_output(std::shared_ptr<IOutput> output) override {m_output = output;}

      // Input is expecting a CVPixelBufferRef
      void PushBuffer(const uint8_t* const data, size_t size, const IMetadata& metadata) override;

      void stop() override;

    public:

      void set_bitrate(int bitrate) override;

      const int bitrate() const override { return m_bitrate; }

    public:
      void set_epoch(const std::chrono::steady_clock::time_point epoch) override {}

    public:

      bool setUpWriter();

    private:
      void stopPushEncode();

      const char *GetRandomPath();
    private:
      std::weak_ptr<IOutput> m_output;

      int m_bitrate;

      unsigned int m_frameW;
      unsigned int m_frameH;

      int m_fps;
      int m_frameCount;

      int m_y_size;

      dispatch_queue_t m_queue;

      int m_framecnt;

    private:
      AVFormatContext* fmt_ctx_;
      AVCodecContext* codec_ctx_;
      AVStream* video_sem_;
      AVFrame* frame_;
      AVOutputFormat* output_fmt_;
      AVCodec* m_codec;
      AVPacket m_pkt;


      uint8_t* m_pic_buf;
    };
  }
}


#endif /* H264Encode_hpp */
