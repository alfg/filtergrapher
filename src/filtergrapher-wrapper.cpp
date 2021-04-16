#include <vector>
#include <string>
#include <vector>
#include <inttypes.h>
#include <emscripten.h>
#include <emscripten/bind.h>

using namespace emscripten;

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <libavutil/opt.h>
};

const char *filter_descr = "eq=contrast=1.75";

static AVFormatContext *fmt_ctx;
static AVCodecContext *dec_ctx;
AVFilterContext *buffersink_ctx;
AVFilterContext *buffersrc_ctx;
AVFilterGraph *filter_graph;
static int video_stream_index = -1;

const std::string c_avformat_version()
{
    return AV_STRINGIFY(LIBAVFORMAT_VERSION);
}

const std::string c_avcodec_version()
{
    return AV_STRINGIFY(LIBAVCODEC_VERSION);
}

const std::string c_avutil_version()
{
    return AV_STRINGIFY(LIBAVUTIL_VERSION);
}

static void save_ppm_frame(unsigned char *buf, int wrap, int xsize, int ysize, char *filename);

static int init_filters(const char *filters_descr)
{
    av_log_set_level(AV_LOG_TRACE);

    char args[512];
    int ret = 0;
    const AVFilter *buffersrc = avfilter_get_by_name("buffer");
    const AVFilter *buffersink = avfilter_get_by_name("buffersink");
    AVFilterInOut *outputs = avfilter_inout_alloc();
    AVFilterInOut *inputs = avfilter_inout_alloc();
    AVRational time_base = fmt_ctx->streams[video_stream_index]->time_base;
    enum AVPixelFormat pix_fmts[] = {AV_PIX_FMT_RGB24, AV_PIX_FMT_GRAY8, AV_PIX_FMT_NONE};
    filter_graph = avfilter_graph_alloc();
    if (!outputs || !inputs || !filter_graph)
    {
        printf("ERROR");
        // ret = AVERROR(ENOMEM);
        // goto end;
    }

    /* buffer video source: the decoded frames from the decoder will be inserted here. */
    snprintf(args, sizeof(args),
             "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
             dec_ctx->width, dec_ctx->height, dec_ctx->pix_fmt,
             time_base.num, time_base.den,
             dec_ctx->sample_aspect_ratio.num, dec_ctx->sample_aspect_ratio.den);

    printf("create_filter %s \n", args);

    if (avfilter_graph_create_filter(&buffersrc_ctx, buffersrc, "in", args, NULL, filter_graph) != 0) {
        printf("create_filter error\n");
    }

    printf("rets \n");

    if (ret < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "Cannot create buffer source\n");
        goto end;
    }
    /* buffer video sink: to terminate the filter chain. */
    ret = avfilter_graph_create_filter(&buffersink_ctx, buffersink, "out",
                                       NULL, NULL, filter_graph);
    if (ret < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "Cannot create buffer sink\n");
        goto end;
    }
    ret = av_opt_set_int_list(buffersink_ctx, "pix_fmts", pix_fmts,
                              AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN);
    if (ret < 0)
    {
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
    if ((ret = avfilter_graph_parse_ptr(filter_graph, filters_descr,
                                        &inputs, &outputs, NULL)) < 0)
        goto end;
    if ((ret = avfilter_graph_config(filter_graph, NULL)) < 0)
        goto end;
end:
    avfilter_inout_free(&inputs);
    avfilter_inout_free(&outputs);
    return ret;
}

typedef struct FileInfoResponse
{
    int frames;
} FileInfoResponse;

FileInfoResponse get_file_info(std::string filename)
{
    FILE *file = fopen(filename.c_str(), "rb");
    if (!file)
    {
        printf("cannot open file\n");
    }
    fclose(file);

    // Open the file and read header.
    if (avformat_open_input(&fmt_ctx, filename.c_str(), NULL, NULL) != 0)
    {
        printf("ERROR: could not open the file\n");
    }

    printf("format %s, duration %lld us, bit_rate %lld\n", fmt_ctx->iformat->name, fmt_ctx->duration, fmt_ctx->bit_rate);
    printf("finding stream info from format\n");

    // Get the stream info so we can iterate the streams.
    if (avformat_find_stream_info(fmt_ctx, NULL) < 0)
    {
        printf("ERROR could not get the stream info\n");
    }

    AVCodec *pCodec = NULL;
    AVCodecParameters *pCodecParameters = NULL;

    // loop though all the streams and print its main information
    for (int i = 0; i < fmt_ctx->nb_streams; i++)
    {
        AVCodecParameters *pLocalCodecParameters = NULL;
        pLocalCodecParameters = fmt_ctx->streams[i]->codecpar;

        printf("AVStream->time_base before open coded %d/%d \n", fmt_ctx->streams[i]->time_base.num, fmt_ctx->streams[i]->time_base.den);
        printf("AVStream->r_frame_rate before open coded %d/%d \n", fmt_ctx->streams[i]->r_frame_rate.num, fmt_ctx->streams[i]->r_frame_rate.den);
        printf("AVStream->start_time %" PRId64 "\n", fmt_ctx->streams[i]->start_time);
        printf("AVStream->duration %" PRId64 "\n", fmt_ctx->streams[i]->duration);

        AVCodec *pLocalCodec = NULL;

        // finds the registered decoder for a codec ID.
        pLocalCodec = avcodec_find_decoder(pLocalCodecParameters->codec_id);

        if (pLocalCodec == NULL)
        {
            printf("ERROR unsupported codec!\n");
        }

        // When the stream is a video we store its index, codec parameters and codec.
        if (pLocalCodecParameters->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            if (video_stream_index == -1)
            {
                video_stream_index = i;
                pCodec = pLocalCodec;
                pCodecParameters = pLocalCodecParameters;
            }

            printf("Video Codec: resolution %d x %d\n", pLocalCodecParameters->width, pLocalCodecParameters->height);
        }
        else if (pLocalCodecParameters->codec_type == AVMEDIA_TYPE_AUDIO)
        {
            printf("Audio Codec: %d channels, sample rate %d\n", pLocalCodecParameters->channels, pLocalCodecParameters->sample_rate);
        }

        // Print its name, id and bitrate.
        printf("\tCodec %s ID %d bit_rate %lld\n", pLocalCodec->name, pLocalCodec->id, pLocalCodecParameters->bit_rate);
    }

    // Allocate for codec context.
    dec_ctx = avcodec_alloc_context3(pCodec);
    if (!dec_ctx)
    {
        printf("failed to allocated memory for AVCodecContext\n");
    }

    // Fill the codec context based on the values from the supplied codec parameters.
    if (avcodec_parameters_to_context(dec_ctx, pCodecParameters) < 0)
    {
        printf("failed to copy codec params to codec context\n");
    }

    // Initialize the AVCodecContext to use the given AVCodec.
    if (avcodec_open2(dec_ctx, pCodec, NULL) < 0)
    {
        printf("failed to open codec through avcodec_open2\n");
    }

    printf("allocate for frames\n");
    // Allocate for frames.
    AVFrame *pFrame = av_frame_alloc();
    if (!pFrame)
    {
        printf("failed to allocated memory for AVFrame\n");
    }

    // Allocate for RGB frames.
    AVFrame *pFrameRGB = av_frame_alloc();
    if (!pFrameRGB)
    {
        printf("failed to allocated memory for AVFrame\n");
    }

    printf("get image buffer size\n");
    // Get image buffer size.
    int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24,
                                            dec_ctx->width,
                                            dec_ctx->height,
                                            16);

    // Allocate image buffer with size from previous function.
    uint8_t *buffer = NULL;
    buffer = static_cast<uint8_t *>(av_malloc(numBytes));

    printf("fill arrays\n");
    // Fill the buffer with image data needed for swscale.
    av_image_fill_arrays(pFrameRGB->data,
                         pFrameRGB->linesize,
                         buffer,
                         AV_PIX_FMT_RGB24,
                         dec_ctx->width,
                         dec_ctx->height, 1);

    printf("sws_getContext\n");
    // Create the swscale context for the images. We only need to set this once.
    struct SwsContext *sws_ctx = NULL;
    sws_ctx = sws_getContext(
        dec_ctx->width,
        dec_ctx->height,
        dec_ctx->pix_fmt,
        dec_ctx->width,
        dec_ctx->height,
        AV_PIX_FMT_RGB24,
        SWS_BILINEAR,
        NULL, NULL, NULL);

    // Allocate for compressed AVPacket.
    AVPacket *pPacket = av_packet_alloc();
    if (!pPacket)
    {
        printf("failed to allocated memory for AVPacket\n");
    }

    printf("init_filters\n");
    // Initialize the filters.
    //
    // Allocate for filtered frames.
    AVFrame *filt_frame = av_frame_alloc();
    int ret;
    if ((ret = init_filters(filter_descr)) < 0)
    {
        printf("ERROR: %s", av_err2str(ret));
    }

    int response = 0;
    int how_many_packets_to_process = 8;

    printf("read packets\n");

    // Fill the Packet with data from the Stream.
    while (av_read_frame(fmt_ctx, pPacket) >= 0)
    {

        // If it's the video stream.
        if (pPacket->stream_index == video_stream_index)
        {
            printf("AVPacket->pts %" PRId64 "\n", pPacket->pts);

            // Decode the packets to frames and apply sws_scale to populate FrameRGB data.
            int response2 = 0;
            response2 = avcodec_send_packet(dec_ctx, pPacket);
            while (response2 >= 0)
            {
                response2 = avcodec_receive_frame(dec_ctx, pFrame);
                if (response2 == AVERROR(EAGAIN) || response2 == AVERROR_EOF)
                {
                    break;
                }
                else if (response2 < 0)
                {
                    printf("Error while receiving a frame from the decoder: %s\n", av_err2str(response));
                }

                sws_scale(sws_ctx, (uint8_t const *const *)pFrame->data,
                          pFrame->linesize, 0, dec_ctx->height,
                          pFrameRGB->data, pFrameRGB->linesize);

                // Apply the filter.
                //
                // Push the decoded frame into the filtergraph
                if (av_buffersrc_add_frame_flags(buffersrc_ctx, pFrame, AV_BUFFERSRC_FLAG_KEEP_REF) < 0)
                {
                    av_log(NULL, AV_LOG_ERROR, "Error while feeding the filtergraph\n");
                    break;
                }

                // Pull filtered frames from the filtergraph
                while (1)
                {
                    ret = av_buffersink_get_frame(buffersink_ctx, filt_frame);
                    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                        break;
                    if (ret < 0)
                        av_frame_unref(filt_frame);
                }

                // Save the filtered frame into PPM format (raw rgb).
                char frame_filename[1024];
                snprintf(frame_filename, sizeof(frame_filename), "%s-%d.ppm", "frame", dec_ctx->frame_number);
                save_ppm_frame(filt_frame->data[0], filt_frame->linesize[0], dec_ctx->width, dec_ctx->height, frame_filename);
            }

            if (response < 0)
                break;

            // Stop it, otherwise we'll be saving hundreds of frames.
            if (--how_many_packets_to_process <= 0)
                break;
        }

        av_packet_unref(pPacket);
    }

    printf("releasing all the resources\n");
    avformat_close_input(&fmt_ctx);
    av_packet_free(&pPacket);
    av_frame_free(&pFrame);
    avcodec_free_context(&dec_ctx);

    FileInfoResponse r = {
        .frames = 10
    };
    return r;
}

static void save_ppm_frame(unsigned char *buf, int wrap, int xsize, int ysize, char *filename)
{
    printf("save_ppm_frame\n");

    FILE *f;
    int i;
    f = fopen(filename, "w");
    // Writing the minimal required header for a ppm file format.
    // https://en.wikipedia.org/wiki/Netpbm#PPM_example
    fprintf(f, "P6\n%d %d\n%d\n", xsize, ysize, 255);

    // Line by line.
    for (i = 0; i < ysize; i++)
        fwrite(buf + i * wrap, 1, xsize * 3, f);
    fclose(f);
}

EMSCRIPTEN_BINDINGS(constants)
{
    function("avformat_version", &c_avformat_version);
    function("avcodec_version", &c_avcodec_version);
    function("avutil_version", &c_avutil_version);
}

EMSCRIPTEN_BINDINGS(structs)
{
    emscripten::value_object<FileInfoResponse>("FileInfoResponse")
        .field("frames", &FileInfoResponse::frames);
    function("get_file_info", &get_file_info);
}