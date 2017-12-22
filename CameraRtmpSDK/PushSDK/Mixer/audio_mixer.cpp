//
// Created by 何剑聪 on 2017/12/20.
// Copyright (c) 2017 Abson. All rights reserved.
//

#include "audio_mixer.h"

namespace PushSDK {namespace ffmpeg {

    uint64_t OUTPUT_CHANNELS = 1;

    static char *const get_error_text(const int error)
    {
      static char error_buffer[255];
      av_strerror(error, error_buffer, sizeof(error_buffer));
      return error_buffer;
    }

    audio_mixer::audio_mixer(std::vector<std::string>& inputs,
        const std::string &output) : sink_(nullptr),
                                     graph_(nullptr),
                                     output_fmt_ctx_(nullptr),
                                     output_codec_ctx_(nullptr) {

      av_log_set_level(AV_LOG_VERBOSE);

      av_register_all();
      avfilter_register_all();

      // open multiple input file
      for (auto& input : inputs ) {
        AVFormatContext* input_fmt_ctx = nullptr;
        AVCodecContext* input_codec_ctx = nullptr;

        // 初始化 AVFormatContext、AVCodecContext
        if (OpenInputFile(input, &input_fmt_ctx, &input_codec_ctx)) {
          av_log(NULL, AV_LOG_ERROR, "Error while opening file 1\n");
          exit(1);
        }

        av_dump_format(input_fmt_ctx, 0, input.c_str(), 0);

        input_fmt_ctxs_.push_back(input_fmt_ctx);
        input_codec_ctxs_.push_back(input_codec_ctx);
      }

      // Set up the filtergraph.
      int err = InitFilterGraph(&graph_, srcs_, &sink_);
      printf("Init err = %d\n", err);

      remove(output.c_str());

      av_log(nullptr, AV_LOG_INFO, "Output file: %s\n", output.c_str());

      // 打开输出文件，打开输出文件io，设置编码器
      AVCodecContext* input_codec_ctx0 = input_codec_ctxs_[0];
      err = OpenOutputFile(output, input_codec_ctx0, &output_codec_ctx_, &output_fmt_ctx_);
      printf("open output file err : %d\n", err);
      av_dump_format(output_fmt_ctx_, 0, output.c_str(), 1);
    }

    void audio_mixer::StartMixAudio() {

      if (WriteOutputFileHeader(output_fmt_ctx_) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Error while writing header outputfile\n");
        exit(1);
      }

      ProcessAll();

      if(WriteOutputFileTrailer(output_fmt_ctx_) < 0){
        av_log(NULL, AV_LOG_ERROR, "Error while writing header outputfile\n");
        exit(1);
      }

      printf("FINISHED\n");
    }

    int
    audio_mixer::OpenInputFile(const std::string &fname,
        AVFormatContext** input_fmt_ctx,
        AVCodecContext** input_codec_ctx) {

      AVCodec* input_codec;

      int err;

      // 打开输入文件并读取它
      if ((err = avformat_open_input(input_fmt_ctx, fname.c_str(), nullptr, nullptr)) < 0) {
        av_log(nullptr, AV_LOG_ERROR, "Could not open input file '%s' (error '%s')\n",
            fname.c_str(), get_error_text(err));
        *input_fmt_ctx = nullptr;
        return err;
      }

      // 获取流的信息
      if ((err = avformat_find_stream_info((*input_fmt_ctx), nullptr)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Could not open find stream info (error '%s')\n",
            get_error_text(err));
        avformat_close_input(input_fmt_ctx);
        return err;
      }

      // 保证输入文件只有一路流
      if ((*input_fmt_ctx)->nb_streams != 1) {
        av_log(NULL, AV_LOG_ERROR, "Expected one audio input stream, but found %d\n",
            (*input_fmt_ctx)->nb_streams);
        avformat_close_input(input_fmt_ctx);
        return AVERROR_EXIT;
      }

      // 为音频流找出解码器
      if (!(input_codec = avcodec_find_decoder((*input_fmt_ctx)->streams[0]->codec->codec_id))) {
        av_log(NULL, AV_LOG_ERROR, "Could not find input codec\n");
        avformat_close_input(input_fmt_ctx);
        return AVERROR_EXIT;
      }

      // 为音频流打开解码器，在后面会使用到这解码器
      if ((err = avcodec_open2((*input_fmt_ctx)->streams[0]->codec, input_codec, nullptr)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Could not open input codec (error '%s')\n",
            get_error_text(err));
        avformat_close_input(input_fmt_ctx);
        return err;
      }

      // 赋值解码器的上下文
      (*input_codec_ctx) = (*input_fmt_ctx)->streams[0]->codec;

      return 0;
    }

    int
    audio_mixer::InitFilterGraph(AVFilterGraph **graph,
        std::vector<AVFilterContext *>& scrs,
        AVFilterContext **sink) {

      AVFilterGraph* filter_graph;

      AVFilter* mix_filter;
      AVFilterContext *mix_ctx;

      AVFilterContext *abuffersink_ctx;
      AVFilter        *abuffersink;

      size_t nb_inputs = input_fmt_ctxs_.size();

      char args[512];
      int err;

      // 创建一个新的包含所有 filter 的 filtergraph
      filter_graph = avfilter_graph_alloc();
      if (!filter_graph) {
        av_log(NULL, AV_LOG_ERROR, "Unable to create filter graph.\n");
        return AVERROR(ENOMEM);
      }

      /******* Create multiple buffer ********/
//      size_t buffer_count = nb_inputs;

      /* Create the abuffer filter;
       * it will be used for feeding the data into the graph. */
      for (size_t i = 0; i < nb_inputs; ++i) {
        AVFilter* abuffer = nullptr;
        AVFilterContext* abuffer_ctx = nullptr;
        char abuffer_ctx_name[256];

        abuffer = avfilter_get_by_name("abuffer");
        if (!abuffer) {
          av_log(NULL, AV_LOG_ERROR, "Could not find the abuffer filter.\n");
          return AVERROR_FILTER_NOT_FOUND;
        }

        /* buffer audio source: 从解码器解码得到的帧将会被插入到这来. */
        AVCodecContext* input_codec_ctx = input_codec_ctxs_[i];
        if (!(input_codec_ctx->channel_layout)) {
          // 设置编码器声道
          input_codec_ctx->channel_layout = static_cast<uint64_t>(av_get_default_channel_layout(input_codec_ctx->channels));
        }

        // 为滤波器实例设置参数
        snprintf(args, sizeof(args),
            "sample_rate=%d:sample_fmt=%s:channel_layout=0x%" PRIx64,
            input_codec_ctx->sample_rate,
            av_get_sample_fmt_name(input_codec_ctx->sample_fmt), input_codec_ctx->channel_layout);

        // 设置名称
        snprintf(abuffer_ctx_name, sizeof(abuffer_ctx_name), "src%zi", i);

        // 初始化滤波器，通过滤波图表和滤波参数
        err = avfilter_graph_create_filter(&abuffer_ctx, abuffer, abuffer_ctx_name,
            args, NULL, filter_graph);
        if (err < 0) {
          av_log(NULL, AV_LOG_ERROR, "Cannot create audio buffer source\n");
          return err;
        }

        srcs_.push_back(abuffer_ctx);
      }

      /****** amix ******* */
      /* Create mix filter. */
      mix_filter = avfilter_get_by_name("amix");
      if (!mix_filter) {
        av_log(NULL, AV_LOG_ERROR, "Could not find the mix filter.\n");
        return AVERROR_FILTER_NOT_FOUND;
      }

      snprintf(args, sizeof(args), "inputs=%zi", nb_inputs);

      err = avfilter_graph_create_filter(&mix_ctx, mix_filter, "amix", args, NULL, filter_graph);
      if (err < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot create audio amix filter\n");
        return err;
      }

      /* Finally create the abuffersink filter;
      * it will be used to get the filtered data out of the graph. */
      abuffersink = avfilter_get_by_name("abuffersink");
      if (!abuffersink) {
        av_log(NULL, AV_LOG_ERROR, "Could not find the abuffersink filter.\n");
        return AVERROR_FILTER_NOT_FOUND;
      }

      abuffersink_ctx = avfilter_graph_alloc_filter(filter_graph, abuffersink, "sink");
      if (!abuffersink_ctx) {
        av_log(NULL, AV_LOG_ERROR, "Could not allocate the abuffersink instance.\n");
        return AVERROR(ENOMEM);
      }

      /* Same sample fmts as the output file. */
      static const enum AVSampleFormat out_sample_fmts[] = { AV_SAMPLE_FMT_S16, AV_SAMPLE_FMT_NONE};
      err = av_opt_set_int_list(abuffersink_ctx, "sample_fmts", out_sample_fmts, AV_SAMPLE_FMT_NONE, AV_OPT_SEARCH_CHILDREN);

      uint8_t ch_layout[64];
      av_get_channel_layout_string(reinterpret_cast<char *>(ch_layout), sizeof(ch_layout), 0, OUTPUT_CHANNELS);
      av_opt_set    (abuffersink_ctx, "channel_layout", reinterpret_cast<const char *>(ch_layout), AV_OPT_SEARCH_CHILDREN);

      if (err < 0) {
        av_log(NULL, AV_LOG_ERROR, "Could set options to the abuffersink instance.\n");
        return err;
      }

      /* Now initialize the filter; we pass NULL options, since we have already
       * set all the options above. */
      err = avfilter_init_str(abuffersink_ctx, nullptr);
      if (err < 0) {
        av_log(NULL, AV_LOG_ERROR, "Could not initialize the abuffersink instance.\n");
        return err;
      }

      for (size_t j = 0; j < nb_inputs; ++j) {
        AVFilterContext* abuffer_ctx = srcs_[j];

        err = avfilter_link(abuffer_ctx, 0, mix_ctx, j);
        if (err < 0) { break;}
      }
      if (err >= 0)
        err = avfilter_link(mix_ctx, 0, abuffersink_ctx, 0);
      if (err < 0) {
        av_log(NULL, AV_LOG_ERROR, "Error connecting filters\n");
        return err;
      }

      // Configure the graph
      err = avfilter_graph_config(filter_graph, nullptr);
      if (err < 0) {
        av_log(NULL, AV_LOG_ERROR, "Error while configuring graph : %s\n", get_error_text(err));
        return err;
      }

      // 描绘出 FilterGraph 这个图， 类似于：
      /*
                       [main]
input --> split ---------------------> overlay --> output
            |                             ^
            |[tmp]                  [flip]|
            +-----> crop --> vflip -------+
       *
       * */
      char* dump = avfilter_graph_dump(filter_graph, nullptr);
      av_log(nullptr, AV_LOG_ERROR, "Graph:\n%s\n", dump);

      *graph = filter_graph;
      *sink = abuffersink_ctx;

      return 0;
    }

    int
    audio_mixer::OpenOutputFile(const std::string& fname,
        AVCodecContext* input_codec_ctx,
        AVCodecContext** output_codec_ctx,
        AVFormatContext** output_fmt_ctx) {

      AVIOContext* output_io_ctx = nullptr;
      AVStream* stream = nullptr;
      AVCodec* output_codec = nullptr;
      int error;

      /*Open the output file write to it*/
      if ((error = avio_open(&output_io_ctx, fname.c_str(), AVIO_FLAG_WRITE)) < 0) {
        av_log(nullptr, AV_LOG_ERROR, "Could not open output file '%s' (error '%s')\n",
            fname.c_str(), get_error_text(error));
        return error;
      }

      /*Create a new format context for the output container format*/
      if (!((*output_fmt_ctx) = avformat_alloc_context())) {
        av_log(nullptr, AV_LOG_ERROR, "Could not allocate output format context\n");
        return AVERROR(ENOMEM);
      }

      /* Associate the output file (pointer) with the container format context. */
      (*output_fmt_ctx)->pb = output_io_ctx;

      if (!((*output_fmt_ctx)->oformat = av_guess_format(nullptr, fname.c_str(), nullptr))) {
        av_log(NULL, AV_LOG_ERROR, "Could not find output file format\n");
        goto cleanup;
      }

      av_strlcpy((*output_fmt_ctx)->filename, fname.c_str(), 1024 * sizeof(char));

      /* Find the encoder to be used by its name. */
      if (!(output_codec = avcodec_find_encoder(AV_CODEC_ID_PCM_S16LE))) {
        av_log(NULL, AV_LOG_ERROR, "Could not find an PCM encoder.\n");
        goto cleanup;
      }

      /* Create a new audio stream in the output file container. */
      if (!(stream = avformat_new_stream((*output_fmt_ctx), output_codec))) {
        av_log(NULL, AV_LOG_ERROR, "Could not create new stream\n");
        error = AVERROR(ENOMEM);
        goto cleanup;
      }

      /* ave the encoder context for easiert access later.*/
      (*output_codec_ctx) = stream->codec;

      /* Set the basic encoder parameters */
      (*output_codec_ctx)->channels = static_cast<int>(OUTPUT_CHANNELS);
      (*output_codec_ctx)->channel_layout = static_cast<uint64_t>(av_get_default_channel_layout(static_cast<int>(OUTPUT_CHANNELS)));
      (*output_codec_ctx)->sample_rate = input_codec_ctx->sample_rate;
      (*output_codec_ctx)->sample_fmt = AV_SAMPLE_FMT_S16;

      av_log(NULL, AV_LOG_INFO, "output bitrate %lld\n", (*output_codec_ctx)->bit_rate);

      /*
      * Some container formats (like MP4) require global headers to be present
      * Mark the encoder so that it behaves accordingly.
      */
      if (((*output_fmt_ctx)->oformat->flags & AVFMT_GLOBALHEADER)) {
        (*output_codec_ctx)->flags |= CODEC_FLAG_GLOBAL_HEADER;
      }

      /* Open the encoder for the audio stream to use it later. */
      if ((error = avcodec_open2((*output_codec_ctx), output_codec, nullptr)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Could not open output codec (error '%s')\n",
            get_error_text(error));
        goto cleanup;
      }

      return 0;

      cleanup:
      avio_close((*output_fmt_ctx)->pb);
      avformat_free_context((*output_fmt_ctx));
      (*output_fmt_ctx) = nullptr;
      return error < 0 ? error : AVERROR_EXIT;
    }

    int audio_mixer::ProcessAll() {
      int ret = 0;

      int data_present = 0;
      int finished = 0;

      size_t nb_inputs = input_fmt_ctxs_.size();

      AVFormatContext* input_fmt_ctxs[nb_inputs];
      AVCodecContext* input_codec_ctxs[nb_inputs];

      AVFilterContext* buffer_ctxs[nb_inputs];

      int input_finished[nb_inputs];
//
      int input_to_read[nb_inputs];
//
      int total_samples[nb_inputs];

      for (int j = 0; j < nb_inputs; ++j) {
        input_finished[j] = 0;
        input_to_read[j] = 1;
        total_samples[j] = 0;

        input_fmt_ctxs[j] = input_fmt_ctxs_[j];
        input_codec_ctxs[j] = input_codec_ctxs_[j];
        buffer_ctxs[j] = srcs_[j];
      }

      int total_out_samples = 0;

      int nb_finished = 0;

      while (nb_finished < nb_inputs) {
        int data_present_in_graph = 0;

        for(int i = 0 ; i < nb_inputs ; i++){
          if(input_finished[i] || input_to_read[i] == 0){
            continue;
          }

          input_to_read[i] = 0;

          AVFrame *frame = NULL;

          if(InitInputFrame(&frame) > 0){
            goto end;
          }

          /* Decode one frame for worth of audio samples. */
          if ((ret = DecodeAudioFrame(frame, input_fmt_ctxs[i], input_codec_ctxs[i], &data_present, &finished))) {
            goto end;
          }

          /*
           * If we are at the end of the file and there are no more samples
           * in the decoder which are delayed, we are actually finished.
           * This must not be treated as an error.
           */
          if (finished && !data_present) {
            input_finished[i] = 1;
            nb_finished++;
            ret = 0;
            av_log(NULL, AV_LOG_INFO, "Input n°%d finished. Write NULL frame \n", i);

            ret = av_buffersrc_write_frame(buffer_ctxs[i], NULL);
            if (ret < 0) {
              av_log(NULL, AV_LOG_ERROR, "Error writing EOF null frame for input %d\n", i);
              goto end;
            }
          }
          else if (data_present) { /** If there is decoded data, convert and store it */
            /* push the audio data from decoded frame into the filtergraph */
            ret = av_buffersrc_write_frame(buffer_ctxs[i], frame);
            if (ret < 0) {
              av_log(NULL, AV_LOG_ERROR, "Error while feeding the audio filtergraph\n");
              goto end;
            }

            av_log(NULL, AV_LOG_INFO, "add %d samples on input %d (%d Hz, time=%f, ttime=%f)\n",
                frame->nb_samples, i, input_codec_ctxs[i]->sample_rate,
                (double)frame->nb_samples / input_codec_ctxs[i]->sample_rate,
                (double)(total_samples[i] += frame->nb_samples) / input_codec_ctxs[i]->sample_rate);

          }

          av_frame_free(&frame);

          data_present_in_graph = data_present | data_present_in_graph;
        }

        if(data_present_in_graph){
          AVFrame *filt_frame = av_frame_alloc();

          /* pull filtered audio from the filtergraph */
          while (1) {
            ret = av_buffersink_get_frame(sink_, filt_frame);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF){
              for(int i = 0 ; i < nb_inputs ; i++){
                if(av_buffersrc_get_nb_failed_requests(buffer_ctxs[i]) > 0){
                  input_to_read[i] = 1;
                  av_log(NULL, AV_LOG_INFO, "Need to read input %d\n", i);
                }
              }

              break;
            }
            if (ret < 0)
              goto end;

            av_log(NULL, AV_LOG_INFO, "remove %d samples from sink (%d Hz, time=%f, ttime=%f)\n",
                filt_frame->nb_samples, output_codec_ctx_->sample_rate,
                (double)filt_frame->nb_samples / output_codec_ctx_->sample_rate,
                (double)(total_out_samples += filt_frame->nb_samples) / output_codec_ctx_->sample_rate);

            //av_log(NULL, AV_LOG_INFO, "Data read from graph\n");
            ret = EncodeAudioFrame(filt_frame, output_fmt_ctx_, output_codec_ctx_, &data_present);
            if (ret < 0)
              goto end;

            av_frame_unref(filt_frame);
          }

          av_frame_free(&filt_frame);
        }
        else {
          av_log(NULL, AV_LOG_INFO, "No data in graph\n");
          for(int i = 0 ; i < nb_inputs ; i++){
            input_to_read[i] = 1;
          }
        }
      }

      return 0;

      end:

      if (ret < 0 && ret != AVERROR_EOF) {
        av_log(NULL, AV_LOG_ERROR, "Error occurred: %s\n", av_err2str(ret));
        exit(1);
      }
      exit(0);
    }

    int
    audio_mixer::WriteOutputFileHeader(AVFormatContext* output_fmt_ctx) {
      int error;
      if ((error = avformat_write_header(output_fmt_ctx, nullptr)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Could not write output file header (error '%s')\n",
            get_error_text(error));
        return error;
      }
      return 0;
    }

    int
    audio_mixer::WriteOutputFileTrailer(AVFormatContext *output_fmt_ctx) {
      int error;
      if ((error = av_write_trailer(output_fmt_ctx)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Could not write output file header (error '%s')\n",
            get_error_text(error));
        return error;
      }
      return 0;
    }

    /* Initialize one audio frame for reading from the input file */
    int
    audio_mixer::InitInputFrame(AVFrame** frame){

      if (!(*frame = av_frame_alloc())) {
        av_log(NULL, AV_LOG_ERROR, "Could not allocate input frame\n");
        return AVERROR(ENOMEM);
      }
      return 0;
    }

    /* Initialize one data packet for reading or writing. */
    void
    audio_mixer::InitPacket(AVPacket* packet) {
      av_init_packet(packet);
      /** Set the packet data and size so that it is recognized as being empty. */
      packet->data = NULL;
      packet->size = 0;
    }

    /* Decode one audio frame from the input file. */
    int
    audio_mixer::DecodeAudioFrame(AVFrame* frame,
        AVFormatContext *input_fmt_ctx,
        AVCodecContext* input_codec_ctx,
        int *data_present,
        int *finished) {

      /** Packet used for temporary storage. */
      AVPacket input_packet;
      int error;
      InitPacket(&input_packet);

      /** Read one audio frame from the input file into a temporary packet. */
      if ((error = av_read_frame(input_fmt_ctx, &input_packet)) < 0) {
        /** If we are the the end of the file, flush the decoder below. */
        if (error == AVERROR_EOF)
          *finished = 1;
        else {
          av_log(NULL, AV_LOG_ERROR, "Could not read frame (error '%s')\n",
              get_error_text(error));
          return error;
        }
      }

      /**
       * Decode the audio frame stored in the temporary packet.
       * The input audio stream decoder is used to do this.
       * If we are at the end of the file, pass an empty packet to the decoder
       * to flush it.
       */
      if ((error = avcodec_decode_audio4(input_codec_ctx, frame,
          data_present, &input_packet)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Could not decode frame (error '%s')\n",
            get_error_text(error));
        av_packet_unref(&input_packet);
        return error;
      }

      /**
       * If the decoder has not been flushed completely, we are not finished,
       * so that this function has to be called again.
       */
      if (*finished && *data_present)
        *finished = 0;
      av_packet_unref(&input_packet);
      return 0;
    }

    int
    audio_mixer::EncodeAudioFrame(AVFrame* frame,
        AVFormatContext* output_fmt_ctx,
        AVCodecContext* output_codec_ctx,
        int *data_present) {

      /** Packet used for temporary storage. */
      AVPacket output_packet;
      int error;
      InitPacket(&output_packet);

      /**
       * Encode the audio frame and store it in the temporary packet.
       * The output audio stream encoder is used to do this.
       */
      if ((error = avcodec_encode_audio2(output_codec_ctx, &output_packet,
          frame, data_present)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Could not encode frame (error '%s')\n",
            get_error_text(error));
        av_packet_unref(&output_packet);
        return error;
      }

      /** Write one audio frame from the temporary packet to the output file. */
      if (*data_present) {
        if ((error = av_write_frame(output_fmt_ctx, &output_packet)) < 0) {
          av_log(NULL, AV_LOG_ERROR, "Could not write frame (error '%s')\n",
              get_error_text(error));
          av_packet_unref(&output_packet);
          return error;
        }

        av_packet_unref(&output_packet);
      }

      return 0;
    }
  }
}
