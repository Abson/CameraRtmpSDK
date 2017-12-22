//
// Created by 何剑聪 on 2017/12/20.
// Copyright (c) 2017 Abson. All rights reserved.
//

#ifndef CAMERARTMPSDK_AUIDO_MIXER_H
#define CAMERARTMPSDK_AUIDO_MIXER_H

#include <string>
#include <array>
#include <vector>

#ifdef __cplusplus
extern "C" {
#endif

#include <libavutil/log.h>
#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/avfiltergraph.h>
#include <libavutil/opt.h>
#include <libavutil/avstring.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>

#ifdef __cplusplus
};
#endif


namespace PushSDK {namespace ffmpeg {

    class audio_mixer {
    public:

//       audio_mixer(const std::string& left_input,
//          const std::string& right_input2,
//          const std::string& output);

       audio_mixer(std::vector<std::string>& inputs, const std::string& output);

      void StartMixAudio();

    private:
//      AVFormatContext* input_fmt_ctx0_;
//      AVFormatContext* input_fmt_ctx1_;
      AVFormatContext* output_fmt_ctx_;
      std::vector<AVFormatContext*> input_fmt_ctxs_;

//      AVCodecContext* input_codec_ctx0_;
//      AVCodecContext* input_codec_ctx1_;
      AVCodecContext* output_codec_ctx_;
      std::vector<AVCodecContext*> input_codec_ctxs_;

//      AVFilterContext* scr0_;
//      AVFilterContext* scr1_;
      AVFilterContext* sink_;
      std::vector<AVFilterContext*> srcs_;

      AVFilterGraph* graph_;

      int OpenInputFile(const std::string &fname,
          AVFormatContext** input_fmt_ctx,
          AVCodecContext** input_codec_ctx);

      int InitFilterGraph(AVFilterGraph **graph,
           std::vector<AVFilterContext *>& scrs,
          AVFilterContext **sink);

      int OpenOutputFile(const std::string& filename,
          AVCodecContext* input_codec_ctx,
          AVCodecContext** output_codec_ctx,
          AVFormatContext** output_fmt_ctx);

      int WriteOutputFileHeader(AVFormatContext*output_fmt_ctx);

      int WriteOutputFileTrailer(AVFormatContext *output_fmt_ctx);

      int ProcessAll();

      int InitInputFrame(AVFrame **frame);

      void InitPacket(AVPacket* packet);

      int DecodeAudioFrame(AVFrame* frame,
          AVFormatContext *input_fmt_ctx,
          AVCodecContext* input_codec_ctx,
          int *data_present,
          int *finished);

      int EncodeAudioFrame(AVFrame *frame, AVFormatContext *output_fmt_ctx, AVCodecContext *output_codec_ctx, int *data_present);
    };

  }
}


#endif //CAMERARTMPSDK_AUIDO_MIXER_H
