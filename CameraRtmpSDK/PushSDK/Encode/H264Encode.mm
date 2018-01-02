//
//  H264Encode.cpp
//  CameraRtmpSDK
//
//  Created by Abson on 16/9/6.
//  Copyright © 2016年 Abson. All rights reserved.
//

#include "H264Encode.h"
#include <CoreMedia/CoreMedia.h>
#include "PixelBuffer.hpp"
#import <Foundation/Foundation.h>
#import <iostream>
#import "H264Parser.hpp"

using namespace push_sdk;
using namespace std;

static uint8_t * _nv12_to_yuv420p	(	uint8_t * 	data,
    unsigned 	width,
    unsigned 	height
)
{
  uint8_t * buf_source = data;
  int len_target = (width * height * 3) / 2; // 对于 yuv420p 来讲 (3/2 * 宽 * 高) 为一帧。
  uint8_t * buf_target = (uint8_t *) av_malloc(len_target * sizeof(uint8_t));

  memcpy(buf_target, buf_source, width * height); // 对于 nv12 来讲，(宽 * 高) 为一帧

  unsigned i;
  for (i = 0 ; i < (width * height / 4) ; i++) {
    buf_target[(width * height) + i] = buf_source[(width * height) + 2 * i];
    buf_target[(width * height) + i + (width * height / 4)] = buf_source[(width * height) + 2 * i + 1];
  }

  return buf_target;
}


namespace push_sdk { namespace  ffmpeg {


    // 输入文件读取完毕后，输出编码器中剩余的 AVPacket
    int flush_encoder(AVFormatContext *fmt_ctx,unsigned int stream_index)
    {
      int ret;
      int got_frame;
      AVPacket enc_pkt;
      if (!(fmt_ctx->streams[stream_index]->codec->codec->capabilities &
          CODEC_CAP_DELAY))
        return 0;

      while (1) {
        enc_pkt.data = NULL;
        enc_pkt.size = 0;
        av_init_packet(&enc_pkt);
        ret = avcodec_encode_video2 (fmt_ctx->streams[stream_index]->codec, &enc_pkt,
            NULL, &got_frame);
        av_frame_free(NULL);
        if (ret < 0)
          break;
        if (!got_frame){
          ret=0;
          break;
        }
        printf("Flush Encoder: Succeed to encode 1 frame!\tsize:%5d\n",enc_pkt.size);
        /* mux encoded frame */
        ret = av_write_frame(fmt_ctx, &enc_pkt);
        if (ret < 0)
          break;
      }
      return ret;
    }

    H264Encode::H264Encode( unsigned frame_w, unsigned frame_h, int fps, int bitrate):
        m_bitrate(bitrate),
        m_frameW(frame_w),
        m_frameH(frame_h),
        m_fps(fps),
        m_queue(dispatch_queue_create("com.PushSdk.ffmpeg.h264", 0)),
        fmt_ctx_(nullptr),
        codec_ctx_(nullptr),
        video_sem_(nullptr),
        frame_(nullptr),
        m_codec(nullptr),
        m_pic_buf(nullptr),
        output_fmt_(nullptr)
    {

      m_framecnt = 0;

      // 注册 ffmpeg所有的编解码器
      av_register_all();

      const char* ch264Path = GetRandomPath();

      //Method1.
      fmt_ctx_ = avformat_alloc_context();
      //Guess Format
      output_fmt_ = av_guess_format(NULL, ch264Path, NULL);
      fmt_ctx_->oformat = output_fmt_;

      if (avio_open(&fmt_ctx_->pb, ch264Path, AVIO_FLAG_READ_WRITE) < 0) {
        cout << "failed to open output file " << endl;
        exit(1);
      }

      video_sem_ = avformat_new_stream(fmt_ctx_, 0);
      video_sem_->time_base = (AVRational){1, fps};
      if (video_sem_==NULL){
        exit(1);
      }

      codec_ctx_ = video_sem_->codec;
      codec_ctx_->codec_id = output_fmt_->video_codec;
      codec_ctx_->pix_fmt = AV_PIX_FMT_YUV420P;
      codec_ctx_->codec_type = AVMEDIA_TYPE_VIDEO;
      codec_ctx_->width = m_frameW;
      codec_ctx_->height = m_frameH;
      codec_ctx_->bit_rate = bitrate;
      codec_ctx_->gop_size = m_fps * 2;
      codec_ctx_->time_base = (AVRational){1, fps};
      // H264
      codec_ctx_->qmin = 10;
      codec_ctx_->qmax = 51;

      // optional param
      codec_ctx_->max_b_frames = 1;

      // 由于PAL制式每秒有25帧（N制为30帧），如果是用于实时视频，如电视、网上视频等，GOP长度应在15至25之间。这样可以在一秒内完成视频快进或回退。
      // codec_ctx_->gop_size = 10;

      // H.264
      AVDictionary *param = 0;
      if(codec_ctx_->codec_id == AV_CODEC_ID_H264) {

        av_dict_set(&param, "preset", "slow", 0);
        av_dict_set(&param, "tune", "zerolatency", 0);
      }

      av_dump_format(fmt_ctx_, 0, ch264Path, 1);

      // codec
      m_codec = avcodec_find_encoder(codec_ctx_->codec_id);
      if (!m_codec) {
        cout << "can not find encdoe" << endl;
        exit(1);
      }

      if (avcodec_open2(codec_ctx_, m_codec, &param) < 0) {
        cout << "open codec failure " << endl;
        exit(1);
      }

      // frame
      frame_ = av_frame_alloc();
      frame_->format = codec_ctx_->pix_fmt;
      frame_->width  = codec_ctx_->width;
      frame_->height = codec_ctx_->height;

      int ret;
      /* the image can be allocated by any means and av_image_alloc() is
       * just the most convenient way if av_malloc() is to be used */
      ret = av_image_alloc(frame_->data, frame_->linesize, codec_ctx_->width, codec_ctx_->height,
          codec_ctx_->pix_fmt, 32);
      if (ret < 0) {
        fprintf(stderr, "Could not allocate raw picture buffer\n");
        exit(1);
      }

      // write
      ret = avformat_write_header(fmt_ctx_, nullptr);
      if (ret < 0) {
        printf("can not write header\n");
        exit(1);
      }

      // packet
      av_new_packet(&m_pkt, 0);

      m_y_size = codec_ctx_->width * codec_ctx_->height;
    }

    H264Encode::~H264Encode()
    {

    }

    const char* H264Encode::GetRandomPath() {
      NSString *date = nil;
      NSDateFormatter *formatter = [[NSDateFormatter alloc] init];
      formatter.dateFormat = @"YYYY-MM-dd hh:mm:ss";
      date = [formatter stringFromDate:[NSDate date]];
      const char* ch264Path = [[NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask,YES).lastObject stringByAppendingPathComponent:[NSString stringWithFormat:@"/%@.h264", date]] cStringUsingEncoding:NSUTF8StringEncoding];
      return ch264Path;
    }

    void
    H264Encode::pushBuffer(const uint8_t* const data, size_t size, const IMetadata& metadata)
    {
      CVPixelBufferRef imageBuffer = (CVPixelBufferRef)data;

      // 必须先 init packet ，否则会报 packet is too small 的 bug
      //        av_init_packet(&pkt_);
      //        pkt_.data = nullptr;
      //        pkt_.size = 0;

      //        Apple::PixelBuffer* pb = (Apple::PixelBuffer*)data;
      //        CVPixelBufferRef pb = (CVPixelBufferRef)data;

      // cover N12 to YUV420
      unsigned int width = m_frameW;
      unsigned int height = m_frameH;
      UInt8 *bufferPtr = (UInt8 *)CVPixelBufferGetBaseAddressOfPlane(imageBuffer,0);
      uint8_t* yuv420_data = _nv12_to_yuv420p(bufferPtr, width, height);

      m_pic_buf = yuv420_data;
      frame_->data[0] = m_pic_buf; // 'Y'
      frame_->data[1] = m_pic_buf + m_y_size;  // 'U'
      frame_->data[2] = m_pic_buf + m_y_size * 5/4; // 'V'

      // PTS
      // frame_->pts = (1 / m_fps) * (m_bitrate / 1000) * m_framecnt;
      frame_->pts = m_framecnt;
      int got_picture = 0;

      // encode
      frame_->width = width;
      frame_->height = height;
      frame_->format = AV_PIX_FMT_YUV420P;

      // should handle PTS and Delay

      int ret = avcodec_encode_video2(codec_ctx_, &m_pkt, frame_, &got_picture);
      if(ret < 0) {
        printf("Failed to encode! \n");
        exit(1);
      }

      if (got_picture == 1)
      {
        m_framecnt++;
        m_pkt.stream_index = video_sem_->index;  // index 用来标识 音/视 频

        // TODO: 添加将 H264 封装成 YUV

        ret = av_write_frame(fmt_ctx_, &m_pkt);
        if (ret == 0) {
          printf("Succeed to encode frame: %5d\tsize:%5d\t type:\n", m_framecnt  - 1, m_pkt.size);
        }
        av_packet_unref(&m_pkt);
      }

      av_free(yuv420_data);
    }

    void
    H264Encode::setBitrate(int bitrate)
    {
      if (m_bitrate == bitrate) { return;}

      m_bitrate = bitrate;
    }


    void
    H264Encode::stopPushEncode()
    {
      int ret = flush_encoder(fmt_ctx_, 0);
      if (ret < 0) {
        printf("Flushing encoder failed\n");
        exit(1);
      }

      av_write_trailer(fmt_ctx_);

      // clear
      if (video_sem_) {
        avcodec_close(video_sem_->codec);
        av_free(frame_);
      }

      avio_close(fmt_ctx_->pb);
      av_free(fmt_ctx_);
    }

    void
    H264Encode::stop()
    {
      stopPushEncode();
    }
  }
}






















