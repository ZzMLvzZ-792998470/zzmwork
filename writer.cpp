#include "writer.h"


Writer::Writer(AVFormatContext *ofmt_ctx) : ofmt_ctx(ofmt_ctx) {


}


Writer::~Writer() {


}

int Writer::write_header() {
    int ret = avformat_write_header(ofmt_ctx, nullptr);
    if(ret < 0){
        av_log(nullptr, AV_LOG_ERROR, "Error occurred when begining(write_header) output file\n");
        return ret;
    }

    return 0;
}


int Writer::write_packets(AVPacket *enc_pkt) {
    int ret = av_interleaved_write_frame(ofmt_ctx, enc_pkt);
    if(ret < 0){
        av_log(nullptr, AV_LOG_ERROR, "Error occurred when during(write_frame) output file\n");
        return ret;
    }

    return 0;
}


int Writer::write_tail() {
    int ret = av_write_trailer(ofmt_ctx);
    if(ret != 0){
        if(ret < 0){
            av_log(nullptr, AV_LOG_ERROR, "Error occurred when ending(write_tailer) output file\n");
            return ret;
        }
    }
    return 0;
}



//
//int Writer::init_ofmt() {
//
//
////    /* init muxer, write output file header */
////    ret = avformat_write_header(ofmt_ctx, nullptr); //---->time_base---->15360
////    if (ret < 0) {
////        av_log(nullptr, AV_LOG_ERROR, "Error occurred when opening output file\n");
////        return ret;
////    }
//
//
//}