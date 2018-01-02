//
//  AACEncoder.cpp
//  CameraRtmpSDK
//
//  Created by Abson on 2016/11/23.
//  Copyright © 2016年 Abson. All rights reserved.
//

#include "AACEncoder.hpp"
//#include <stdio.h>
#include "swresample.h"

inline static bool check_sample_fmt(AVCodec* codec, enum AVSampleFormat sample_fmt)
{
  const enum AVSampleFormat* p = codec->sample_fmts;
  //    while (*p != AV_SAMPLE_FMT_NONE) {
  //        if (*p == sample_fmt) {
  //            return true;
  //        }
  //        p++;
  //    }
  int i = 0;
  while (p[i] != AV_SAMPLE_FMT_NONE) {
    if (p[i] == sample_fmt) {
      return true;
    }
    i++;
  }
  return false;
}

inline static int select_sample_rate(AVCodec* codec)
{
  const int* p;
  int best_samplerate = 0;
  if (!codec->supported_samplerates) {
    return 44100;
  }

  p = codec->supported_samplerates;
  while (*p) {
    best_samplerate = FFMAX(*p, best_samplerate);
    p++;
  }
  return best_samplerate;
}

static uint64_t select_channel_layout(AVCodec* codec)
{
  const uint64_t* p;
  uint64_t best_ch_layout = 0;
  int best_nb_channels = 0;

  if (!codec->channel_layouts) {
    return AV_CH_LAYOUT_STEREO;
  }
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

inline static int flush_encoder(AVCodecContext* ctx, AVPacket& pkt, AVFormatContext* fmtCtx, unsigned int stream_index)
{
  int got_output = 0;
  int i = 0;
  int ret = 0;

  if (fmtCtx) {
    if (!(fmtCtx->streams[stream_index]->codec->codec->capabilities &
        CODEC_CAP_DELAY)){
      return 0;
    }
    while (1) {
      pkt.data = NULL;
      pkt.size = 0;
      av_init_packet(&pkt);
      ret = avcodec_encode_audio2 (fmtCtx->streams[stream_index]->codec, &pkt,
          NULL, &got_output);
      av_frame_free(NULL);
      if (ret < 0)
        break;
      if (!got_output){
        ret=0;
        break;
      }
      printf("Flush Encoder: Succeed to encode 1 frame!\tsize:%5d\n",pkt.size);
      /* mux encoded frame */
      ret = av_write_frame(fmtCtx, &pkt);
      if (ret < 0)
        break;
    }
    return ret;
  }

  for (got_output = 1; got_output; i++) {
    ret = avcodec_encode_audio2(ctx, &pkt, NULL, &got_output);
    if (ret < 0) {
      fprintf(stderr, "Error encoding frame\n");
      exit(1);
    }
    if (got_output) {
      printf("Succeed to encode 1 frame! \tsize:%5d\n",pkt.size);
      if (fmtCtx) {

      }
      av_packet_unref(&pkt);
    }
  }

  return ret;
}

namespace push_sdk { namespace ffmpeg {

    void
    AACEncoder::initAudioResample()
    {
      // 由 AV_SAMPLE_FMT_S16 转为 AV_SAMPLE_FMT_FLTP
      swr_ctx_ = swr_alloc_set_opts(nullptr,
          av_get_default_channel_layout(code_ctx_->channels),
          code_ctx_->sample_fmt,
          code_ctx_->sample_rate,
          av_get_default_channel_layout(code_ctx_->channels),
          AV_SAMPLE_FMT_S16,
          code_ctx_->sample_rate,
          0, nullptr);
      if (!swr_ctx_) {
        fprintf(stderr, "Could not allocate resampler context\n");
        exit(1);
      }
      int ret = swr_init(swr_ctx_);
      if (ret < 0) {
        fprintf(stderr, "Failed to initialize the resampling context\n");
        exit(1);
      }
      // 分配空间
      convert_data_ = (uint8_t**)calloc(code_ctx_->channels, sizeof(*convert_data_));

      ret = av_samples_alloc(convert_data_, nullptr, code_ctx_->channels, code_ctx_->frame_size, code_ctx_->sample_fmt, 0);
      if (ret < 0) {
        fprintf(stderr, "Failed av_samples_alloc\n");
        exit(1);
      }
    }

    void
    AACEncoder::audioResample(const AVFrame& coverFrame)
    {
      /* 转换数据，令各自声道的音频数据存储在不同的数组（分别由不同指针指向）*/
      swr_convert(swr_ctx_, (uint8_t**)coverFrame.data, coverFrame.nb_samples,
          (const uint8_t**)(frame_->data), frame_->nb_samples);

      // 将转换后的数据复制给 frame
//        int length = code_ctx_->frame_size * av_get_bytes_per_sample(code_ctx_->sample_fmt);

//        for (int k = 0; k < 2; ++k){
//            for (int j = 0; j < length; ++j)
//            {
//                frame_->data[k][j] = convert_data_[k][j];
//            }
//        }

    }

    //    static float t = 0;
    //    static float tincr = 0;

    AACEncoder::AACEncoder(int frequencyInHz, int channelCount, int bitrate, const std::string& filename)
        :codec_(nullptr), code_ctx_(nullptr), frame_(nullptr), m_birerate(bitrate),m_file(nullptr),
         m_outfmt(nullptr), m_stream(nullptr), m_fmtCtx(nullptr), samples_(nullptr), convert_data_(nullptr)
    {
      bool DEBUGMODE = filename.length() > 1;

      int buffer_size = 0, ret = 0;

      // 注册 ffmpeg所有的编解码器
      av_register_all();

      codec_ = avcodec_find_encoder(AV_CODEC_ID_AAC);
      if (!codec_) {
        std::cout << "AV_CODEC_ID_AAC not found\n";
        exit(1);
      }

      if (DEBUGMODE) {
        m_fmtCtx = avformat_alloc_context();
        m_outfmt = av_guess_format(NULL, filename.c_str(), NULL);
        m_fmtCtx->oformat = m_outfmt;
        if (avio_open(&m_fmtCtx->pb, filename.c_str(), AVIO_FLAG_READ_WRITE) < 0) {
          std::cout << "failed to open output file \n";
          exit(1);
        }
        m_stream = avformat_new_stream(m_fmtCtx, codec_);
        if (m_stream == nullptr) {
          std::cout << "failed to new audio stream  \n";
          exit(1);
        }
      }

      if (DEBUGMODE) {
        code_ctx_ = m_stream->codec;
        code_ctx_->codec_id = m_outfmt->audio_codec;
      } else {
        code_ctx_ = avcodec_alloc_context3(codec_);
      }
      if (!code_ctx_) {
        std::cout << "Could not allocate audio codec context\n";
        exit(1);
      }

      code_ctx_->codec_type = AVMEDIA_TYPE_AUDIO;
      // put sample parameters, 500 kbps–1.4 Mbps —44.1KHz的无损音频，解码器为FLAC Audio,WavPack或Monkey's Audio
      code_ctx_->bit_rate = m_birerate;

      // check that the encoder supports fltp pcm input
      code_ctx_->sample_fmt = AV_SAMPLE_FMT_FLTP;
      if (false == check_sample_fmt(codec_, code_ctx_->sample_fmt)) {
        printf("Encoder does not support sample format %s", av_get_sample_fmt_name(code_ctx_->sample_fmt));
        exit(1);
      }

      // select other audio parameter supported by the encoder
      code_ctx_->sample_rate = frequencyInHz; // 采样率，aac 视频直播通常采用 44100 Hz, 最低可采用 8k
      code_ctx_->channel_layout = AV_CH_LAYOUT_STEREO; // 双声道
      code_ctx_->channels = av_get_channel_layout_nb_channels(code_ctx_->channel_layout);
      code_ctx_->bit_rate = bitrate;

      if (DEBUGMODE) {
        av_dump_format(m_fmtCtx, 0, filename.c_str(), 1);
      }

      // open it
      if (avcodec_open2(code_ctx_, codec_, nullptr) < 0) {
        fprintf(stderr, "Could not open codec\n");
        exit(1);
      }

      if (filename.length() > 1) {
        m_file = std::fopen(filename.c_str(), "wb");
        if (!m_file) {
          fprintf(stderr, "Could not open %s\n", filename.c_str());
          exit(1);
        }
      }

      frame_ = av_frame_alloc();
      if (!frame_) {
        fprintf(stderr, "Could not allocate audio frame\n");
        exit(1);
      }
      frame_->nb_samples = code_ctx_->frame_size; // 1024
      frame_->format = code_ctx_->sample_fmt;
      frame_->channel_layout = code_ctx_->channel_layout;
      frame_->channels = av_get_channel_layout_nb_channels(code_ctx_->channel_layout);
      frame_->sample_rate = code_ctx_->sample_rate;

      AACEncoder::initAudioResample();

      // the codec gives us the frame size, in samples, we calculate the size of the samples buffer in bytes
      buffer_size = av_samples_get_buffer_size(NULL, code_ctx_->channels, code_ctx_->frame_size, code_ctx_->sample_fmt, 0);
      if (buffer_size < 0) {
        fprintf(stderr, "Could not get sample buffer size\n");
        exit(1);
      }

      samples_ = static_cast<uint8_t*>( av_malloc(buffer_size));
      if (!samples_) {
        fprintf(stderr, "Could not allocate %d bytes for samples buffer\n",
            buffer_size);
        exit(1);
      }

      // setup the data pointer in the AVFrame
      ret = avcodec_fill_audio_frame(frame_, code_ctx_->channels, code_ctx_->sample_fmt, reinterpret_cast<const uint8_t*>(samples_), buffer_size, 0);
      if (ret < 0) {
        fprintf(stderr, "Could not setup audio frame\n");
        exit(1);
      }

      // Write Header
      if (DEBUGMODE) {
        ret = avformat_write_header(m_fmtCtx, nullptr);
        if (ret != 0) {
          fprintf(stderr, "Could not write aac audio header\n");
          exit(1);
        }
      }
      //        tincr = 2 * M_PI * 440.0 / code_ctx_->sample_rate;
    }

    AACEncoder::~AACEncoder()
    {
      avcodec_close(code_ctx_);
      av_freep(&frame_->data[0]);
      av_free(frame_);
      av_free(samples_);
      av_freep(&convert_data_[0]);
      av_free(convert_data_);
    }

    void
    AACEncoder::setOutput(std::shared_ptr<IOutput> output)
    {
      m_output = output;
    }

    void
    AACEncoder::pushBuffer(const uint8_t *const data, size_t size, const IMetadata &metadata)
    {
      av_init_packet(&pkt_);
      pkt_.data = nullptr;
      pkt_.size = 0;

      memcpy(samples_, data, size);

      frame_->data[0] = const_cast<uint8_t*>(samples_);

      conver_frame_ = create_av_frame(code_ctx_->frame_size,
          code_ctx_->sample_fmt,
          code_ctx_->channel_layout,
          code_ctx_->sample_rate);

      AACEncoder::audioResample(*conver_frame_);

      conver_frame_->pts = m_framecnt * 100;

      int got_frame = 0;
      // encode
      int ret = avcodec_encode_audio2(code_ctx_, &pkt_, conver_frame_, &got_frame);
      if (ret < 0) {
        fprintf(stderr, "Error encoding audio frame\n");
        exit(1);
      }
      if (got_frame==1){
        printf("Succeed to encode 1 frame! \tsize:%5d\n",pkt_.size);
        if (m_fmtCtx) {
          //                std::fwrite(pkt_.data, 1, pkt_.size, m_file);
          pkt_.stream_index = m_stream->index;
          ret = av_write_frame(m_fmtCtx, &pkt_);
        }
        av_packet_unref(&pkt_);
        m_framecnt++;
      }
    }

    void
    AACEncoder::stop()
    {
      int ret = flush_encoder(code_ctx_, pkt_, m_fmtCtx, 0);
      if (ret < 0) {
        fprintf(stderr, "Flushing encoder failed\n");
        exit(1);
      }

      if (m_fmtCtx) {
        av_write_trailer(m_fmtCtx);
        if (m_stream) {
          avcodec_close(m_stream->codec);
          av_free(frame_);
          av_free(samples_);
          avio_close(m_fmtCtx->pb);
          avformat_free_context(m_fmtCtx);
        }
      }
    }

    AVFrame*
    AACEncoder::create_av_frame( const int nb_samples,
        enum AVSampleFormat sample_fmt,
        uint64_t channel_layout,
        int sample_rate)
    {
      AVFrame* frame = av_frame_alloc();
      frame->format = sample_fmt;
      frame->channel_layout = channel_layout;
      frame->channels = av_get_channel_layout_nb_channels(frame->channel_layout);
      frame->sample_rate = sample_rate;
      frame->nb_samples =nb_samples; // 1024
      if (nb_samples) {
        int ret = av_frame_get_buffer(frame, 0); // 通过这个方法，根据 channel_layout 和 nb_samples 来填充好 frame.data 的缓存空间
        if (ret < 0) {
          fprintf(stderr, "Error allocating an audio buffer\n");
          av_frame_free(&frame);
          return nullptr;
        }
      }
      return frame;
    }
  }
}



