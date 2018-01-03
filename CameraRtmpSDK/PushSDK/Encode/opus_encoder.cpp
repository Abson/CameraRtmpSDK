//
// Created by 何剑聪 on 2017/12/27.
// Copyright (c) 2017 Abson. All rights reserved.
//

#include "opus_encoder.h"
#include "utility.h"
#include "avassert.h"

namespace push_sdk { namespace ffmpeg {


    OpusEncoder::OpusEncoder(int frequencyInHz,
        int channelCount,
        int bitrate,
        const std::string& filename) : output_file_(nullptr), stream_(nullptr), output_fmt_ctx_(nullptr),
                                       output_codec_ctx_(nullptr), sample_rate_(frequencyInHz),
                                       bit_rate_(bitrate) {

      av_register_all();

      int erro;
      erro = OpenOutputFile(&output_fmt_ctx_, &output_file_, &output_codec_ctx_, &stream_,filename);
      printf("open output file err : %d\n", erro);

      erro = WriteOutputFileHeader(output_fmt_ctx_);
      if (erro != 0) {
        av_log(nullptr, AV_LOG_ERROR, "failure to write output file header");
        exit(1);
      }

      erro = CreatePacket(&packet_);
      if (erro != 0) {
        av_log(nullptr, AV_LOG_ERROR, "failure to create packet");
        exit(1);
      }

      erro = InitFrame(&frame_, output_codec_ctx_);
      if (erro != 0) {
        av_log(nullptr, AV_LOG_ERROR, "failure to create frame");
        exit(1);
      }

      int linesize = av_get_bytes_per_sample(output_codec_ctx_->sample_fmt) * output_codec_ctx_->frame_size;
      printf("linesize is %d\n", linesize);
    }

    int
    OpusEncoder::OpenOutputFile( AVFormatContext** output_fmt_ctx, FILE** output_file,
        AVCodecContext** output_codec_ctx, AVStream** output_stream,
        const std::string &fname) {

      AVCodecContext *avctx          = NULL;
      AVIOContext *output_io_context = NULL;
      AVStream *stream               = NULL;
      AVCodec *output_codec          = NULL;
      int error;

      /* Open the output file to write to it. */
      if ((error = avio_open(&output_io_context, fname.c_str(),
          AVIO_FLAG_WRITE)) < 0) {
        fprintf(stderr, "Could not open output file '%s' (error '%s')\n",
            fname.c_str(), av_err2str(error));
        return error;
      }

      /* Create a new format context for the output container format. */
      if (!(*output_fmt_ctx = avformat_alloc_context())) {
        fprintf(stderr, "Could not allocate output format context\n");
        return AVERROR(ENOMEM);
      }

      /* Associate the output file (pointer) with the container format context. */
      (*output_fmt_ctx)->pb = output_io_context;

      /* Guess the desired container format based on the file extension. */
      if (!((*output_fmt_ctx)->oformat = av_guess_format(NULL, fname.c_str(),
          NULL))) {
        fprintf(stderr, "Could not find output file format\n");
        goto cleanup;
      }

      av_strlcpy((*output_fmt_ctx)->filename, fname.c_str(),
          sizeof((*output_fmt_ctx)->filename));

      /* Find the encoder to be used by its name. */
      if (!(output_codec = avcodec_find_encoder(AV_CODEC_ID_OPUS))) {
        fprintf(stderr, "Could not find an AAC encoder.\n");
        goto cleanup;
      }

      /* Create a new audio stream in the output file container. */
      if (!(stream = avformat_new_stream(*output_fmt_ctx, NULL))) {
        fprintf(stderr, "Could not create new stream\n");
        error = AVERROR(ENOMEM);
        goto cleanup;
      }

      avctx = avcodec_alloc_context3(output_codec);
      if (!avctx) {
        fprintf(stderr, "Could not allocate an encoding context\n");
        error = AVERROR(ENOMEM);
        goto cleanup;
      }

      /* Set the basic encoder parameters.
       * The input file's sample rate is used to avoid a sample rate conversion. */
      // avctx->channel_layout = static_cast<uint64_t>(av_get_default_channel_layout());
      avctx->channel_layout = AV_CH_LAYOUT_STEREO;
      avctx->channels       = av_get_channel_layout_nb_channels(avctx->channel_layout);
      avctx->sample_rate    = sample_rate_;
      avctx->sample_fmt     = AV_SAMPLE_FMT_S16;
      avctx->bit_rate       = bit_rate_;


      /* Allow the use of the experimental AAC encoder. */
      avctx->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;

      /* Set the sample rate for the container. */
      stream->time_base.den = avctx->sample_rate;
      stream->time_base.num = 1;

      avctx->codec_tag = 0;
      /* Some container formats (like MP4) require global headers to be present.
       * Mark the encoder so that it behaves accordingly. */
      if ((*output_fmt_ctx)->oformat->flags & AVFMT_GLOBALHEADER)
        avctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

      /* Open the encoder for the audio stream to use it later. */
      if ((error = avcodec_open2(avctx, output_codec, NULL)) < 0) {
        fprintf(stderr, "Could not open output codec (error '%s')\n",
            av_err2str(error));
        goto cleanup;
      }

      error = avcodec_parameters_from_context(stream->codecpar, avctx);
      if (error < 0) {
        fprintf(stderr, "Could not initialize stream parameters\n");
        goto cleanup;
      }

      /* Save the encoder context for easier access later. */
      *output_codec_ctx = avctx;
      *output_stream = stream;

      return 0;

      cleanup:
      avcodec_free_context(&avctx);
      avio_closep(&(*output_fmt_ctx)->pb);
      avformat_free_context(*output_fmt_ctx);
      *output_fmt_ctx = NULL;
      return error < 0 ? error : AVERROR_EXIT;
    }

    void
    OpusEncoder::pushBuffer(const uint8_t *const data, size_t size, const IMetadata &metadata) {

      int data_present = 0;

      buffer_queue_.Write(data, size);

      while ( buffer_queue_.availableBytes() >= GetBytesPerFrame(output_codec_ctx_)) {

        size_t bytes_pre_frame = static_cast<size_t>(GetBytesPerFrame(output_codec_ctx_));
        EncodeAudioFrame(buffer_queue_.Read(bytes_pre_frame), frame_, output_file_, output_fmt_ctx_, output_codec_ctx_, &data_present);
      }
    }

    int
    OpusEncoder::EncodeAudioFrame(uint8_t *buffer, AVFrame *frame, FILE *output_file,
        AVFormatContext *output_fmt_ctx, AVCodecContext *output_codec_ctx, int *data_present) {

      /* Packet used for temporary storage. */
      AVPacket output_packet;
      int error;
      InitPacket(&output_packet);

      if (frame) {

        frame->data[0] = buffer;

        printf("\n");

        /* Set a timestamp based on the sample rate for the container. */
        frame->pts = pts_;
        pts_ += frame->nb_samples;
      }

      /* Encode the audio frame and store it in the temporary packet.
       * The output audio stream encoder is used to do this. */
      if ((error = avcodec_encode_audio2(output_codec_ctx, &output_packet,
          frame, data_present)) < 0) {
        fprintf(stderr, "Could not encode frame (error '%s')\n",
            av_err2str(error));
        av_packet_unref(&output_packet);
        return error;
      }

      /* Write one audio frame from the temporary packet to the output file. */
      if (data_present) {
        output_packet.stream_index = stream_->index;

        error = WriteOutputFileFrame(output_fmt_ctx, output_packet);
        if (error != 0) {
          fprintf(stderr, "Could not write a frame (error '%s')\n",
              av_err2str(error));
          return error;
        }
      }

      return 0;
    }

    int
    OpusEncoder::CreatePacket(AVPacket** packet) {
      *packet = av_packet_alloc();
      /** Set the packet data and size so that it is recognized as being empty. */
      if (!(*packet)) {
        fprintf(stderr, "could not allocate the packet\n");
        exit(1);
      }
      (*packet)->size = 0;
      (*packet)->data = nullptr;
      return 0;
    }

    int
    OpusEncoder::InitFrame(AVFrame **frame, AVCodecContext *output_codec_ctx) {
      int err;

      if (!( (*frame) = av_frame_alloc() )) {
        av_log(NULL, AV_LOG_ERROR, "Could not allocate audio frame\n");
        exit(1);
      }

      (*frame)->nb_samples = output_codec_ctx->frame_size;
      (*frame)->format = output_codec_ctx->sample_fmt;
      (*frame)->channel_layout = output_codec_ctx->channel_layout;
      (*frame)->sample_rate    = output_codec_ctx->sample_rate;

      /** alloc the data buffers*/
      err = av_frame_get_buffer(*frame, 0);
      if (err < 0) {
        av_log(NULL, AV_LOG_ERROR, "Could not allocate audio data buffers\n");
        exit(1);
      }

      return 0;
    }

    /**
     * Initialize one data packet for reading or writing.
     * @param packet Packet to be initialized
     */
    void OpusEncoder::InitPacket(AVPacket *packet)
    {
      av_init_packet(packet);
      /* Set the packet data and size so that it is recognized as being empty. */
      packet->data = NULL;
      packet->size = 0;
    }

    void
    OpusEncoder::stop() {

      buffer_queue_.ThrowAway();

      /* flush the encoder */
      flush_encoder(0);

      WriteOutputFileTrailer(output_fmt_ctx_);

      fclose(output_file_);
      av_frame_free(&frame_);
      av_packet_free(&packet_);
      avcodec_close(output_codec_ctx_);
      avcodec_free_context(&output_codec_ctx_);
      avformat_free_context(output_fmt_ctx_);
    }

    int
    OpusEncoder::flush_encoder(unsigned int stream_index)
    {
      int ret;
      int got_frame;
      AVPacket enc_pkt;
      if (!(output_codec_ctx_->codec->capabilities &
          CODEC_CAP_DELAY))
        return 0;
      while (1) {
        enc_pkt.data = NULL;
        enc_pkt.size = 0;
        av_init_packet(&enc_pkt);
        ret = avcodec_encode_audio2 (output_codec_ctx_, &enc_pkt,
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
        ret = av_write_frame(output_fmt_ctx_, &enc_pkt);
        if (ret < 0) {
          break;
        }
      }
      return ret;
    }

    int
    OpusEncoder::WriteOutputFileTrailer(AVFormatContext *output_fmt_ctx) {
      int error;
      if ((error = av_write_trailer(output_fmt_ctx)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Could not write output file header (error '%s')\n",
            utility::get_error_text(error));
        return error;
      }
      return 0;
    }

    int
    OpusEncoder::WriteOutputFileHeader(AVFormatContext* output_fmt_ctx) {
      int error;
      if ((error = avformat_write_header(output_fmt_ctx, nullptr)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Could not write output file header (error '%s')\n",
            utility::get_error_text(error));
        return error;
      }
      return 0;
    }

    int
    OpusEncoder::WriteOutputFileFrame(AVFormatContext* output_fmt_ctx, AVPacket& output_packet) {
      int error;
      if ((error = av_write_frame(output_fmt_ctx, &output_packet)) < 0) {
        fprintf(stderr, "Could not write frame (error '%s')\n",
            av_err2str(error));
        av_packet_unref(&output_packet);
        return error;
      }

      av_log(nullptr, AV_LOG_INFO, "Succeed to encode 1 frame! \tsize:%5d\n",output_packet.size);

      av_packet_unref(&output_packet);

      return 0;
    }

    int
    OpusEncoder::GetBytesPerFrame( AVCodecContext* output_codec_ctx) {
      return av_samples_get_buffer_size(nullptr, output_codec_ctx->channels,
          output_codec_ctx->frame_size, output_codec_ctx->sample_fmt, 0);
    }

    int OpusEncoder::Test(const char *pcm_in_file) {
      FILE* in_file= nullptr;
      in_file= fopen(pcm_in_file, "rb");

      int size = GetBytesPerFrame(output_codec_ctx_);
      uint8_t* frame_buf = (uint8_t *)av_malloc(static_cast<size_t>(size));

      int got_frame;
      for (int i = 0; i < 1000; ++i) {
        if (fread(frame_buf, 1, (size_t)size, in_file) <= 0){
          printf("Failed to read raw data! \n");
          return -1;
        }
        else if(feof(in_file)){
          break;
        }
        got_frame=0;
        EncodeAudioFrame(frame_buf, frame_, nullptr, output_fmt_ctx_, output_codec_ctx_, &got_frame);
      }

      //Flush Encoder
      int ret = flush_encoder(0);
      if (ret < 0) {
        printf("Flushing encoder failed\n");
        return -1;
      }

      stop();
      av_free(frame_buf);

      fclose(in_file);

      return 0;
    }
  }
}
