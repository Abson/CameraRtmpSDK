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

using namespace PushSDK;
using namespace std;

static uint8_t * _nv12_to_yuv420p	(	uint8_t * 	data,
                                     unsigned 	width,
                                     unsigned 	height
                                     )
{
    uint8_t * buf_source = data;
    int len_target = (width * height * 3) / 2;
    uint8_t * buf_target = (uint8_t *) av_malloc(len_target * sizeof(uint8_t));

    memcpy(buf_target, buf_source, width * height);

    unsigned i;
    for (i = 0 ; i < (width * height / 4) ; i++) {
        buf_target[(width * height) + i] = buf_source[(width * height) + 2 * i];
        buf_target[(width * height) + (width * height / 4) + i] = buf_source[(width * height) + 2 * i + 1];
    }

    return buf_target;
}


namespace PushSDK { namespace  ffmpeg {


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

    H264Encode::H264Encode( int frame_w, int frame_h, int fps, int bitrate):
    m_bitrate(bitrate),
    m_frameW(frame_w),
    m_frameH(frame_h),
    m_fps(fps),
    m_queue(dispatch_queue_create("com.PushSdk.ffmpeg.h264", 0)),
    m_fmt_ctx(nullptr),
    m_codec_ctx(nullptr),
    m_video_st(nullptr),
    m_frame(nullptr),
    m_codec(nullptr),
    m_pic_buf(nullptr),
    m_out_fmt(nullptr)
    {

        m_framecnt = 0;

        // 注册 ffmpeg所有的编解码器
        av_register_all();

        m_fmt_ctx = avformat_alloc_context();

        NSString *date = nil;
        NSDateFormatter *formatter = [[NSDateFormatter alloc] init];
        formatter.dateFormat = @"YYYY-MM-dd hh:mm:ss";
        date = [formatter stringFromDate:[NSDate date]];
        const char* ch264Path = [[NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask,YES).lastObject stringByAppendingPathComponent:[NSString stringWithFormat:@"/%@.h264", date]] cStringUsingEncoding:NSUTF8StringEncoding];
        //Method1.
        m_fmt_ctx = avformat_alloc_context();
        //Guess Format
        m_out_fmt = av_guess_format(NULL, ch264Path, NULL);
        m_fmt_ctx->oformat = m_out_fmt;

        if (avio_open(&m_fmt_ctx->pb, ch264Path, AVIO_FLAG_READ_WRITE) < 0) {
            cout << "failed to open output file " << endl;
            exit(1);
        }

        m_video_st = avformat_new_stream(m_fmt_ctx, 0);
        m_video_st->time_base = (AVRational){1, fps};
        if (m_video_st==NULL){
            exit(1);
        }

        m_codec_ctx = m_video_st->codec;
        m_codec_ctx->codec_id = m_out_fmt->video_codec;
        m_codec_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
        m_codec_ctx->codec_type = AVMEDIA_TYPE_VIDEO;
        m_codec_ctx->width = m_frameW;
        m_codec_ctx->height = m_frameH;
        m_codec_ctx->bit_rate = bitrate;
        m_codec_ctx->gop_size = m_fps * 2;
        m_codec_ctx->time_base = (AVRational){1, fps};
        // H264
        m_codec_ctx->qmin = 10;
        m_codec_ctx->qmax = 51;

        // optional param
        m_codec_ctx->max_b_frames = 1;

        // 由于PAL制式每秒有25帧（N制为30帧），如果是用于实时视频，如电视、网上视频等，GOP长度应在15至25之间。这样可以在一秒内完成视频快进或回退。
        // m_codec_ctx->gop_size = 10;

        // H.264
        AVDictionary *param = 0;
        if(m_codec_ctx->codec_id == AV_CODEC_ID_H264) {

            av_dict_set(&param, "preset", "slow", 0);
            av_dict_set(&param, "tune", "zerolatency", 0);
        }

        av_dump_format(m_fmt_ctx, 0, ch264Path, 1);

        // codec
        m_codec = avcodec_find_encoder(m_codec_ctx->codec_id);
        if (!m_codec) {
            cout << "can not find encdoe" << endl;
            exit(1);
        }

        if (avcodec_open2(m_codec_ctx, m_codec, &param) < 0) {
            cout << "open codec failure " << endl;
            exit(1);
        }

        // frame
        m_frame = av_frame_alloc();
        m_frame->format = m_codec_ctx->pix_fmt;
        m_frame->width  = m_codec_ctx->width;
        m_frame->height = m_codec_ctx->height;

        int ret;
        /* the image can be allocated by any means and av_image_alloc() is
         * just the most convenient way if av_malloc() is to be used */
        ret = av_image_alloc(m_frame->data, m_frame->linesize, m_codec_ctx->width, m_codec_ctx->height,
                             m_codec_ctx->pix_fmt, 32);
        if (ret < 0) {
            fprintf(stderr, "Could not allocate raw picture buffer\n");
            exit(1);
        }

        // write
        ret = avformat_write_header(m_fmt_ctx, nullptr);
        if (ret < 0) {
            printf("can not write header\n");
            exit(1);
        }


        // packet
        av_new_packet(&m_pkt, 0);

        m_y_size = m_codec_ctx->width * m_codec_ctx->height;
    }

    H264Encode::~H264Encode()
    {

    }

    void
    H264Encode::pushBuffer(const uint8_t* const data, size_t size)
    {
        CVPixelBufferRef imageBuffer = (CVPixelBufferRef)data;

        // 必须先 init packet ，否则会报 packet is too small 的 bug
        //        av_init_packet(&m_pkt);
        //        m_pkt.data = nullptr;
        //        m_pkt.size = 0;

        //        Apple::PixelBuffer* pb = (Apple::PixelBuffer*)data;
        //        CVPixelBufferRef pb = (CVPixelBufferRef)data;

        // cover N12 to YUV420
        int width = m_frameW;
        int height = m_frameH;
        UInt8 *bufferPtr = (UInt8 *)CVPixelBufferGetBaseAddressOfPlane(imageBuffer,0);
        uint8_t* yuv420_data = _nv12_to_yuv420p(bufferPtr, width, height);

        m_pic_buf = yuv420_data;
        m_frame->data[0] = m_pic_buf; // 'Y'
        m_frame->data[1] = m_pic_buf + m_y_size;  // 'U'
        m_frame->data[2] = m_pic_buf + m_y_size * 5/4; // 'V'

        // PTS
        // m_frame->pts = (1 / m_fps) * (m_bitrate / 1000) * m_framecnt;
        m_frame->pts = m_framecnt;
        int got_picture = 0;

        // encode
        m_frame->width = width;
        m_frame->height = height;
        m_frame->format = AV_PIX_FMT_YUV420P;

        // should handle PTS and Delay

        int ret = avcodec_encode_video2(m_codec_ctx, &m_pkt, m_frame, &got_picture);
        if(ret < 0) {
            printf("Failed to encode! \n");
            exit(1);
        }

        if (got_picture == 1)
        {
            m_framecnt++;
            m_pkt.stream_index = m_video_st->index;  // index 用来标识 音/视 频

            ret = av_write_frame(m_fmt_ctx, &m_pkt);
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
        int ret = flush_encoder(m_fmt_ctx, 0);
        if (ret < 0) {
            printf("Flushing encoder failed\n");
            exit(1);
        }

        av_write_trailer(m_fmt_ctx);

        // clear
        if (m_video_st) {
            avcodec_close(m_video_st->codec);
            av_free(m_frame);
        }
        
        avio_close(m_fmt_ctx->pb);
        av_free(m_fmt_ctx);
    }

    void
    H264Encode::stopPushBuffer()
    {
        stopPushEncode();
    }
}
}






















