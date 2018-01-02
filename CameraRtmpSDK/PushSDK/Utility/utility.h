//
// Created by 何剑聪 on 2017/12/27.
// Copyright (c) 2017 Abson. All rights reserved.
//

#ifndef CAMERARTMPSDK_UTILITY_H
#define CAMERARTMPSDK_UTILITY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <libavutil/error.h>
#include <libavformat/avformat.h>

#ifdef __cplusplus
};
#endif


namespace push_sdk { namespace ffmpeg {

    namespace utility {

      static char *const get_error_text(const int error) {
        static char error_buffer[255];
        av_strerror(error, error_buffer, sizeof(error_buffer));
        return error_buffer;
      }

      /* check that a given sample format is supported by the encoder */
      static int check_sample_fmt(const AVCodec *codec, enum AVSampleFormat sample_fmt)
      {
        const enum AVSampleFormat *p = codec->sample_fmts;

        while (*p != AV_SAMPLE_FMT_NONE) {
          if (*p == sample_fmt)
            return 1;
          p++;
        }
        return 0;
      }

      /* select layout with the highest channel count */
      static uint64_t select_channel_layout(const AVCodec* codec) {
        const uint64_t *p = nullptr;
        uint64_t best_ch_layout = 0;
        int best_nb_channels = 0;

        if (!codec->channel_layouts)
          return AV_CH_LAYOUT_STEREO;

        p = codec->channel_layouts;
        while (*p) {
          int nb_channels = av_get_channel_layout_nb_channels(*p);

          if (nb_channels > best_ch_layout) {
            best_ch_layout = *p;
            best_nb_channels = nb_channels;
          }
          p++;
        }

        return best_ch_layout;
      }

      /* if sample_rate it  set to zero，just pick the highest supported samplerate */
      static int select_sample_rate(const AVCodec *codec, int sample_rate) {

        if (sample_rate > 0) {
          return sample_rate;
        }

        const int *p;
        int best_sample_rate = 0;

        if (!codec->supported_samplerates)
          return sample_rate;

        p = codec->supported_samplerates;
        while (*p) {
          if (!best_sample_rate || abs(44100 - *p) < abs(44100 - best_sample_rate))
            best_sample_rate = *p;
          p++;
        }
        return best_sample_rate;
      }

      static int get_bytes_per_sample(AVSampleFormat fmt) {
        return av_get_bytes_per_sample(fmt);
      }
    }
  }
}



#endif //CAMERARTMPSDK_UTILITY_H
