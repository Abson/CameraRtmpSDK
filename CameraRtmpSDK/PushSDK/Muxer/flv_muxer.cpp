//
// Created by 何剑聪 on 2017/12/8.
// Copyright (c) 2017 Abson. All rights reserved.
//

#include "flv_muxer.h"
#include <assert.h>

#define STREAM_FRAME_RATE 25 /* 25 images/s */

namespace PushSDK {

  ffmpeg::flv_muxer::flv_muxer(const std::string &input_fname, const std::string &output_fname) {

    OutputStream video_st = { 0 }, audio_st = { 0 };
    const char *filename = nullptr ;
    AVOutputFormat *fmt = nullptr ;
    AVFormatContext *oc = nullptr;
    AVCodec *audio_codec = nullptr , *video_codec = nullptr;
    int ret;
    int have_video = 0, have_audio = 0;
    int encode_video = 0, encode_audio = 0;
    AVDictionary *opt = NULL;
    int i;

    /* Initialize libavcodec, and register all codecs and formats. */
    av_register_all();

    filename = input_fname.c_str();

    avformat_alloc_output_context2(&oc, NULL, NULL, output_fname.c_str());
    if (!oc) {
      printf("Could not deduce output format from file extension: using MPEG.\n");
      avformat_alloc_output_context2(&oc, NULL, "mpeg", filename);
    }
    assert(oc);

    fmt = oc->oformat;

    /* Add the audio and video streams using the default format codecs
     * and initialize the codecs. */
    if(fmt->video_codec != AV_CODEC_ID_NONE) {
      add_stream(&video_st, oc, &video_codec, fmt->video_codec);
      have_video = 1;
      encode_video = 1;
    }
    if(fmt->audio_codec != AV_CODEC_ID_NONE) {
      add_stream(&audio_st, oc, &audio_codec, fmt->audio_codec);
      have_audio = 1;
      encode_audio = 1;
    }

    /* Now that all the parameters are set, we can open the audio and
    * video codecs and allocate the necessary encode buffers. */
    if (have_video)
      open_video(oc, video_codec, &video_st, opt);

    if (have_audio)
      open_audio(oc, audio_codec, &audio_st, opt);
  }

  void
  ffmpeg::flv_muxer::add_stream(
      PushSDK::ffmpeg::OutputStream *ost,
      AVFormatContext *oc,
      AVCodec **codec,
      AVCodecID codec_id) {

    AVCodecContext* c;
    /* find the encoder */
    *codec = avcodec_find_encoder(codec_id);
    if(!(*codec)) {
      fprintf(stderr, "Could not find encoder for '%s'\n",
          avcodec_get_name(codec_id));
      exit(1);
    }

    ost->st = avformat_new_stream(oc, NULL);
    if (!ost->st) {
      fprintf(stderr, "Could not allocate stream\n");
      exit(1);
    }

    ost->st->id = oc->nb_streams-1; // 设置特定于格式的流id，

    c = avcodec_alloc_context3(*codec);
    if (!c) {
      fprintf(stderr, "Could not alloc an encoding context\n");
      exit(1);
    }
    ost->enc = c;

    switch ((*codec)->type ) {

      case AVMEDIA_TYPE_AUDIO:
        c->sample_fmt = (AVSampleFormat) ((*codec)->sample_fmts ?
            (*codec)->sample_fmts[0] : AV_SAMPLE_FMT_FLTP);
        c->bit_rate    = 64000;
        c->sample_rate = 44100;
        if((*codec)->supported_samplerates) {
          c->sample_rate = (*codec)->supported_samplerates[0];
          for (int i = 0; (*codec)->supported_samplerates[i]; i++) {
            if ((*codec)->supported_samplerates[i] == 44100)
              c->sample_rate = 44100;
          }
        }
        c->channels = av_get_channel_layout_nb_channels(c->channel_layout);
        c->channel_layout = AV_CH_LAYOUT_STEREO;
        if ((*codec)->channel_layouts) {
          c->channel_layout = (*codec)->channel_layouts[0];
          for (int i = 0; (*codec)->channel_layouts[i]; i++) {
            if ((*codec)->channel_layouts[i] == AV_CH_LAYOUT_STEREO)
              c->channel_layout = AV_CH_LAYOUT_STEREO;
          }
        }
        c->channels = av_get_channel_layout_nb_channels(c->channel_layout);
        ost->st->time_base = av_make_q(1, c->sample_rate);
        break;
      case AVMEDIA_TYPE_VIDEO:
        c->codec_id = codec_id;

        c->bit_rate = 400000;
        /* Resolution must be a multiple of two. */
        c->width    = 352;
        c->height   = 288;
        /* timebase: This is the fundamental unit of time (in seconds) in terms
         * of which frame timestamps are represented. For fixed-fps content,
         * timebase should be 1/framerate and timestamp increments should be
         * identical to 1. */
        ost->st->time_base = (AVRational){1, STREAM_FRAME_RATE};
        c->time_base = ost->st->time_base;
        c->gop_size      = 12; /* emit one intra frame every twelve frames at most */
        c->pix_fmt       = AV_PIX_FMT_YUV420P;
        if (c->codec_id == AV_CODEC_ID_MPEG2VIDEO) {
          /* just for testing, we also add B-frames */
          c->max_b_frames = 2;
        }
        if (c->codec_id == AV_CODEC_ID_MPEG1VIDEO) {
          /* Needed to avoid using macroblocks in which some coeffs overflow.
           * This does not happen with normal video, it just happens here as
           * the motion of the chroma plane does not match the luma plane. */
          c->mb_decision = 2;
        }
        break;
      default:break;
    }

    /* Some formats want stream headers to be separate. */
    if (oc->oformat->flags & AVFMT_GLOBALHEADER)
      c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
  }

  AVFrame *
  ffmpeg::flv_muxer::alloc_picture(enum AVPixelFormat pix_fmt, int width, int height) {
    AVFrame* picture = nullptr;
    picture = av_frame_alloc();
    if (picture) {
      picture->format = pix_fmt;
      picture->width = width;
      picture->height = height;
    }

    /* allocate the buffers for the frame data */
    int ret = av_frame_get_buffer(picture, 32);
    if (ret < 0) {
      fprintf(stderr, "Could not allocate frame data.\n");
      exit(1);
    }

    return picture;
  }

  void
  ffmpeg::flv_muxer::open_video(AVFormatContext *oc, AVCodec *codec, OutputStream *ost, AVDictionary *opt_arg) {

    int ret;
    AVCodecContext *c = ost->enc;
    AVDictionary *opt = NULL;

    av_dict_copy(&opt, opt_arg, 0);

    /* open the codec */
    ret = avcodec_open2(c, codec, &opt);
    av_dict_free(&opt);
    if (ret < 0) {
      fprintf(stderr, "Could not open video codec: %s\n", av_err2str(ret));
      exit(1);
    }

    /* allocate and init a re-usable frame */
    ost->frame = alloc_picture(c->pix_fmt, c->width, c->height);
    if (!ost->frame) {
      fprintf(stderr, "Could not allocate video frame\n");
      exit(1);
    }

    /* If the output format is not YUV420P, then a temporary YUV420P
     * picture is needed too. It is then converted to the required
     * output format. */
    ost->tmp_frame = NULL;
    if (c->pix_fmt != AV_PIX_FMT_YUV420P) {
      ost->tmp_frame = alloc_picture(AV_PIX_FMT_YUV420P, c->width, c->height);
      if (!ost->tmp_frame) {
        fprintf(stderr, "Could not allocate temporary picture\n");
        exit(1);
      }
    }

//    avcodec_parameters_from_context
  }

  void
  ffmpeg::flv_muxer::open_audio(AVFormatContext *oc, AVCodec *codec, OutputStream *ost, AVDictionary *opt_arg) {
    int ret;
    AVCodecContext *c = ost->enc;
    AVDictionary *opt = NULL;

    av_dict_copy(&opt, opt_arg, 0);
  }
}



