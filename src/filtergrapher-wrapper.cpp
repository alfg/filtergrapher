#include <vector>
#include <string>
#include <vector>
#include <inttypes.h>
#include <emscripten.h>
#include <emscripten/bind.h>

using namespace emscripten;

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
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

typedef struct FileInfoResponse {
    int frames;
} FileInfoResponse;

FileInfoResponse get_file_info(std::string filename) {
    FILE *file = fopen(filename.c_str(), "rb");
    if (!file) {
      printf("cannot open file\n");
    }
    fclose(file);

    FileInfoResponse r = {
        .frames = 10
    };
    return r;
}

EMSCRIPTEN_BINDINGS(constants) {
    function("avformat_version", &c_avformat_version);
    function("avcodec_version", &c_avcodec_version);
    function("avutil_version", &c_avutil_version);
}

EMSCRIPTEN_BINDINGS(structs) {
  emscripten::value_object<FileInfoResponse>("FileInfoResponse")
  .field("frames", &FileInfoResponse::frames)
  ;
  function("get_file_info", &get_file_info);
}