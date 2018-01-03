//
// Created by 何剑聪 on 2017/12/27.
// Copyright (c) 2017 Abson. All rights reserved.
//

#ifndef CAMERARTMPSDK_OPUS_ENCODER_H
#define CAMERARTMPSDK_OPUS_ENCODER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <libavformat/avformat.h>
#include <libavutil/avstring.h>
#include <libswresample/swresample.h>

#ifdef __cplusplus
};
#endif

#include "IEncoder.hpp"
#include "byte_buffer_queue.h"

namespace push_sdk { namespace ffmpeg {

    class OpusEncoder : public IEncoder  {
    public:
      /**
       * @param frequencyInHz 采样率，opus 支持的采样为 8000，12000，16000，24000，48000，但是经过测试，16000以下的都有杂音，
       * 不知道为何，请懂的小伙伴帮忙解决以下
       * @param channelCount 声道
       * @param bitrate 比特率
       * @param filename 编码成功后的文件名，可用 .gg 文件后序，这样使用 chrome 浏览器便可播放
       */
      OpusEncoder(int frequencyInHz = 48000, int channelCount = 2, int bitrate = 16000,
          const std::string& filename = "");

      ~OpusEncoder() {}

    public:
      void set_bitrate(int bitrate) override { bit_rate_ = bitrate; };

      const int bitrate() const override { return bit_rate_; }

    public:
      void set_output(std::shared_ptr<IOutput> output) override { output_ = output; };

    public:
      void set_epoch(const std::chrono::steady_clock::time_point epoch) override {};

      void PushBuffer(const uint8_t *const data, size_t size, const IMetadata &metadata = IMetadata());

      void stop();

    public:
      /* the api for test, you can use the file named "tdjm.pcm" in pcm_in_file parameter*/
      int Test(const char *pcm_in_file);

    private:

      constexpr static uint kOutputChannel = 1;

      int bit_rate_ = 0;
      int sample_rate_ = 0;
      int channels = 0;

      std::weak_ptr<IOutput> output_;

      FILE* output_file_;
      AVFormatContext* output_fmt_ctx_;
      AVCodecContext* output_codec_ctx_;
      AVPacket* packet_;
      AVFrame* frame_;
      AVStream* stream_;

      buffer::BufferQueue buffer_queue_;

      /* Global timestamp for the audio frames. */
      int64_t pts_ = 0;

      int OpenOutputFile(AVFormatContext** output_fmt_ctx, FILE **output_file, AVCodecContext **output_codec_ctx, AVStream** stream, const std::string &fname);

      int CreatePacket(AVPacket** packet);

      int InitFrame(AVFrame **frame, AVCodecContext *output_codec_ctx);

      int EncodeAudioFrame(uint8_t *buffer, AVFrame *frame, AVPacket *packet, AVFormatContext *output_fmt_ctx, AVCodecContext *output_codec_ctx, int *data_present);

      int FlushEncoder(unsigned int stream_index);

      int WriteOutputFileTrailer(AVFormatContext *output_fmt_ctx);

      int WriteOutputFileHeader(AVFormatContext *output_fmt_ctx);

      void InitPacket(AVPacket *packet);

      int GetBytesPerFrame(AVCodecContext *output_codec_ctx);

      int WriteOutputFileFrame(AVFormatContext *output_fmt_ctx, AVPacket &output_packet);
    };
  }
}


#endif //CAMERARTMPSDK_OPUS_ENCODER_H
