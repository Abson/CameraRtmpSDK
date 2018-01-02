//
// Created by 何剑聪 on 2017/12/8.
// Copyright (c) 2017 Abson. All rights reserved.
//

#ifndef CAMERARTMPSDK_FLV_MUXER_H
#define CAMERARTMPSDK_FLV_MUXER_H

#include <string>

#ifdef __cplusplus
extern "C" {
#endif

#include <libavutil/opt.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

#ifdef __cplusplus
}
#endif

namespace push_sdk {namespace ffmpeg {

    typedef struct OutputStream {
      AVStream *st;
      AVCodecContext *enc;

      /* pts of the next frame that will be generated */
      int64_t next_pts;
      int samples_count;

      AVFrame *frame;
      AVFrame *tmp_frame;

      float t, tincr, tincr2;

      struct SwsContext *sws_ctx;
      struct SwrContext *swr_ctx;
    } OutputStream;

    class flv_muxer {
    public:
      flv_muxer(const std::string &input_fname, const std::string &output_fname);

    private:
      // no copy
      flv_muxer(const flv_muxer&);
      void operator=(const flv_muxer&);

      static void open_video(AVFormatContext *oc, AVCodec *codec, OutputStream *ost, AVDictionary *opt_arg);

      inline static void add_stream(OutputStream *ost, AVFormatContext *oc,
          AVCodec **codec,
          enum AVCodecID codec_id);

      static AVFrame *alloc_picture(AVPixelFormat pix_fmt, int width, int height);

      static void open_audio(AVFormatContext *oc, AVCodec *codec, OutputStream *ost, AVDictionary *opt_arg);
    };
}
}


#endif //CAMERARTMPSDK_FLV_MUXER_H
