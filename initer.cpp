#include "initer.h"

Initer::~Initer(){}

IniterI::IniterI(const std::string &filename) : filename(filename) {

}

int IniterI::init_fmt() {
    int ret;

    if ((ret = avformat_open_input(&ifmt_ctx, filename.c_str(), nullptr, nullptr)) < 0) {
        av_log(nullptr, AV_LOG_ERROR, "Cannot open input file\n");
        return ret;
    }

    if ((ret = avformat_find_stream_info(ifmt_ctx, nullptr)) < 0) {
        av_log(nullptr, AV_LOG_ERROR, "Cannot find stream information\n");
        return ret;
    }

    //AVRational base = ifmt_ctx->streams[1]->time_base;
    av_dump_format(ifmt_ctx, 0, filename.c_str(), 0);
    return 0;
}


AVFormatContext* IniterI::get_fmt_ctx() {
    return ifmt_ctx;
}



IniterI::~IniterI(){
    avformat_close_input(&ifmt_ctx);
}



IniterO::IniterO(const std::string &filename) : filename(filename) {

}

IniterO::~IniterO(){
    avformat_free_context(ofmt_ctx);
}


int IniterO::init_fmt() {
    if(filename.substr(0, 4) == "rtmp")  avformat_alloc_output_context2(&ofmt_ctx, nullptr, "flv", filename.c_str());
    else avformat_alloc_output_context2(&ofmt_ctx, nullptr, nullptr, filename.c_str());
    if (!ofmt_ctx) {
        av_log(nullptr, AV_LOG_ERROR, "Could not create output context\n");
        return AVERROR_UNKNOWN;
    }

    return 0;
}

AVFormatContext* IniterO::get_fmt_ctx() {
    return ofmt_ctx;
}



void IniterO::print_ofmt_info() {
    av_dump_format(ofmt_ctx, 0, filename.c_str(), 1);
}


int IniterO::ofmt_io_open() {
    int ret;
    if (!(ofmt_ctx->oformat->flags & AVFMT_NOFILE)) {
        ret = avio_open(&ofmt_ctx->pb, filename.c_str(), AVIO_FLAG_WRITE);
        if (ret < 0) {
            av_log(nullptr, AV_LOG_ERROR, "Could not open output file '%s'", filename.c_str());
            return ret;
        }
    }
    return ret;
}


