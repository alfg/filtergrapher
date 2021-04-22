#include <emscripten.h>
#include <emscripten/bind.h>
#include <inttypes.h>

#include <string>
#include <vector>

using namespace emscripten;

#include "filtergrapher-wrapper.h"

static AVFormatContext *fmt_ctx;
static AVCodecContext *dec_ctx;
AVFilterContext *buffersink_ctx;
AVFilterContext *buffersrc_ctx;
AVFilterGraph *filter_graph;
static int video_stream_index = -1;

static void save_ppm_frame(unsigned char *buf, int wrap, int xsize, int ysize,
                           char *filename) {
  FILE *f;
  int i;
  f = fopen(filename, "w");
  // Writing the minimal required header for a ppm file format.
  // https://en.wikipedia.org/wiki/Netpbm#PPM_example
  fprintf(f, "P6\n%d %d\n%d\n", xsize, ysize, 255);

  // Line by line.
  for (i = 0; i < ysize; i++) fwrite(buf + i * wrap, 1, xsize * 3, f);
  fclose(f);
}

static int init_filters(const char *filters_descr) {
  // av_log_set_level(AV_LOG_TRACE);

  char args[512];
  int ret = 0;
  const AVFilter *buffersrc = avfilter_get_by_name("buffer");
  const AVFilter *buffersink = avfilter_get_by_name("buffersink");
  AVFilterInOut *outputs = avfilter_inout_alloc();
  AVFilterInOut *inputs = avfilter_inout_alloc();
  AVRational time_base = fmt_ctx->streams[video_stream_index]->time_base;
  enum AVPixelFormat pix_fmts[] = {AV_PIX_FMT_RGB24, AV_PIX_FMT_GRAY8,
                                   AV_PIX_FMT_NONE};
  filter_graph = avfilter_graph_alloc();
  if (!outputs || !inputs || !filter_graph) {
    printf("ERROR");
    // ret = AVERROR(ENOMEM);
    // goto end;
  }

  /* buffer video source: the decoded frames from the decoder will be inserted
   * here. */
  snprintf(args, sizeof(args),
           "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
           dec_ctx->width, dec_ctx->height, dec_ctx->pix_fmt, time_base.num,
           time_base.den, dec_ctx->sample_aspect_ratio.num,
           dec_ctx->sample_aspect_ratio.den);

  printf("create_filter %s \n", args);

  if (avfilter_graph_create_filter(&buffersrc_ctx, buffersrc, "in", args, NULL,
                                   filter_graph) != 0) {
    printf("create_filter error\n");
  }

  if (ret < 0) {
    av_log(NULL, AV_LOG_ERROR, "Cannot create buffer source\n");
    goto end;
  }
  /* buffer video sink: to terminate the filter chain. */
  ret = avfilter_graph_create_filter(&buffersink_ctx, buffersink, "out", NULL,
                                     NULL, filter_graph);
  if (ret < 0) {
    av_log(NULL, AV_LOG_ERROR, "Cannot create buffer sink\n");
    goto end;
  }
  ret = av_opt_set_int_list(buffersink_ctx, "pix_fmts", pix_fmts,
                            AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN);
  if (ret < 0) {
    av_log(NULL, AV_LOG_ERROR, "Cannot set output pixel format\n");
    goto end;
  }

  /*
   * Set the endpoints for the filter graph. The filter_graph will
   * be linked to the graph described by filters_descr.
   */
  /*
   * The buffer source output must be connected to the input pad of
   * the first filter described by filters_descr; since the first
   * filter input label is not specified, it is set to "in" by
   * default.
   */
  outputs->name = av_strdup("in");
  outputs->filter_ctx = buffersrc_ctx;
  outputs->pad_idx = 0;
  outputs->next = NULL;
  /*
   * The buffer sink input must be connected to the output pad of
   * the last filter described by filters_descr; since the last
   * filter output label is not specified, it is set to "out" by
   * default.
   */
  inputs->name = av_strdup("out");
  inputs->filter_ctx = buffersink_ctx;
  inputs->pad_idx = 0;
  inputs->next = NULL;
  if ((ret = avfilter_graph_parse_ptr(filter_graph, filters_descr, &inputs,
                                      &outputs, NULL)) < 0)
    goto end;
  if ((ret = avfilter_graph_config(filter_graph, NULL)) < 0) goto end;
end:
  avfilter_inout_free(&inputs);
  avfilter_inout_free(&outputs);
  return ret;
}

static int open_input_file(const char *filename) {
  int ret;
  AVCodec *dec;

  // Open the file and read header.
  if ((ret = avformat_open_input(&fmt_ctx, filename, NULL, NULL)) != 0) {
    printf("ERROR: could not open the file\n");
    av_log(NULL, AV_LOG_ERROR, "Cannot open input file\n");
    return ret;
  }

  // Get the stream info to select the video stream.
  if ((ret = avformat_find_stream_info(fmt_ctx, NULL)) < 0) {
    av_log(NULL, AV_LOG_ERROR, "Cannot find stream information\n");
    return ret;
  }

  ret = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &dec, 0);
  if (ret < 0) {
    av_log(NULL, AV_LOG_ERROR,
           "Cannot find a video stream in the input file\n");
    return ret;
  }
  video_stream_index = ret;

  // Create the decoding context.
  dec_ctx = avcodec_alloc_context3(dec);
  if (!dec_ctx) return AVERROR(ENOMEM);
  avcodec_parameters_to_context(dec_ctx,
                                fmt_ctx->streams[video_stream_index]->codecpar);

  // Init the video decoder.
  if ((ret = avcodec_open2(dec_ctx, dec, NULL)) < 0) {
    av_log(NULL, AV_LOG_ERROR, "Cannot open video decoder\n");
    return ret;
  }
  return 0;
}

Response run_filter(std::string filename, std::string filter, int timestamp) {
  av_log_set_level(AV_LOG_QUIET);

  Response r;

  if (open_input_file(filename.c_str()) < 0) {
    return r;
  }

  AVRational stream_time_base = fmt_ctx->streams[video_stream_index]->time_base;
  r.duration = fmt_ctx->streams[video_stream_index]->duration;
  r.time_base = av_q2d(stream_time_base);

  // If the duration value isn't in the stream, get from the FormatContext.
  if (r.duration == 0) {
    r.duration = fmt_ctx->duration * r.time_base;
  }

  // Allocate for packets, frames, rgb frames and filtered frames.
  AVPacket *packet;
  AVFrame *frame;
  AVFrame *frame_rgb;
  AVFrame *filt_frame;

  packet = av_packet_alloc();
  frame = av_frame_alloc();
  frame_rgb = av_frame_alloc();
  filt_frame = av_frame_alloc();
  if (!frame || !frame_rgb || !filt_frame) {
    perror("Could not allocate frame");
    return r;
  }

  // Get image buffer size for rgb.
  int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, dec_ctx->width,
                                          dec_ctx->height, 16);

  // Allocate image buffer with size from previous function.
  uint8_t *buffer = NULL;
  buffer = static_cast<uint8_t *>(av_malloc(numBytes));

  // Fill the buffer with image data needed for swscale.
  av_image_fill_arrays(frame_rgb->data, frame_rgb->linesize, buffer,
                       AV_PIX_FMT_RGB24, dec_ctx->width, dec_ctx->height, 1);

  // Create the swscale context for the images. We only need to set this once.
  struct SwsContext *sws_ctx = NULL;
  sws_ctx = sws_getContext(dec_ctx->width, dec_ctx->height, dec_ctx->pix_fmt,
                           dec_ctx->width, dec_ctx->height, AV_PIX_FMT_RGB24,
                           SWS_BILINEAR, NULL, NULL, NULL);

  int ret;
  if ((ret = init_filters(filter.c_str())) < 0) {
    printf("ERROR: %s", av_err2str(ret));
    return r;
  }

  int max_packets_to_process = 250;
  int frame_count = 0;
  int key_frames = 0;

  // Seek to frame from the given timestamp.
  av_seek_frame(fmt_ctx, video_stream_index, timestamp, AVSEEK_FLAG_ANY);

  // Read the packets.
  while (av_read_frame(fmt_ctx, packet) >= 0) {
    if (packet->stream_index == video_stream_index) {
      // printf("AVPacket->pts %" PRId64 "\n", packet->pts);

      // Decode the packets to frames and apply sws_scale to populate FrameRGB
      // data.
      int ret = 0;
      ret = avcodec_send_packet(dec_ctx, packet);
      if (ret >= 0) {
        ret = avcodec_receive_frame(dec_ctx, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
          continue;
        }

        // Track keyframes so we paginate by each GOP.
        if (frame->key_frame == 1) key_frames++;

        // Break at the next keyframe found.
        if (key_frames > 1) break;

        sws_scale(sws_ctx, (uint8_t const *const *)frame->data, frame->linesize,
                  0, dec_ctx->height, frame_rgb->data, frame_rgb->linesize);

        // Push the decoded frame into the filtergraph
        if (av_buffersrc_add_frame_flags(buffersrc_ctx, frame,
                                         AV_BUFFERSRC_FLAG_KEEP_REF) < 0) {
          av_log(NULL, AV_LOG_ERROR, "Error while feeding the filtergraph\n");
          break;
        }

        // Pull filtered frames from the filtergraph
        ret = av_buffersink_get_frame(buffersink_ctx, filt_frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;
        if (ret < 0) av_frame_unref(filt_frame);

        // Save the filtered frame into PPM format (raw rgb).
        char frame_filename[1024];
        snprintf(frame_filename, sizeof(frame_filename), "%s-%d.ppm", "frame",
                 dec_ctx->frame_number);
        save_ppm_frame(filt_frame->data[0], filt_frame->linesize[0],
                       dec_ctx->width, dec_ctx->height, frame_filename);

        Frame f = {
          .frame_number = dec_ctx->frame_number,
          .filename = frame_filename,
        };
        r.frames.push_back(f);
        frame_count++;
      }

      // Stop if too many frames are processed.
      if (--max_packets_to_process <= 0) break;
    }
    av_packet_unref(packet);
  }

  r.frame_count = frame_count;

  // Release all resources.
  avfilter_graph_free(&filter_graph);
  avcodec_free_context(&dec_ctx);
  avformat_close_input(&fmt_ctx);
  av_packet_free(&packet);
  av_frame_free(&frame);
  av_frame_free(&frame_rgb);
  av_frame_free(&filt_frame);

  return r;
}

EMSCRIPTEN_BINDINGS(constants) {
  function("avformat_version", &c_avformat_version);
  function("avcodec_version", &c_avcodec_version);
  function("avfilter_version", &c_avfilter_version);
  function("avutil_version", &c_avutil_version);
  function("swscale_version", &c_swscale_version);
}

EMSCRIPTEN_BINDINGS(structs) {
  emscripten::value_object<Frame>("Frame")
      .field("frame_number", &Frame::frame_number)
      .field("filename", &Frame::filename);
  register_vector<Frame>("Frame");

  emscripten::value_object<Response>("Response")
      .field("frames", &Response::frames)
      .field("frame_count", &Response::frame_count)
      .field("duration", &Response::duration)
      .field("time_base", &Response::time_base);
  function("run_filter", &run_filter);
}
