extern "C" {
#include <libavcodec/avcodec.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
};

const std::string c_avformat_version() {
  return AV_STRINGIFY(LIBAVFORMAT_VERSION);
}

const std::string c_avcodec_version() {
  return AV_STRINGIFY(LIBAVCODEC_VERSION);
}

const std::string c_avutil_version() {
  return AV_STRINGIFY(LIBAVUTIL_VERSION);
}

const std::string c_avfilter_version() {
  return AV_STRINGIFY(LIBAVFILTER_VERSION);
}

const std::string c_swscale_version() {
  return AV_STRINGIFY(LIBSWSCALE_VERSION);
}

typedef struct Frame {
  int frame_number;
  std::string filename;
} Frame;

typedef struct Response {
  std::vector<Frame> frames;
  int frame_count;
  int duration;
  double time_base;
} Response;
