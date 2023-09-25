#ifndef _WRITER_H
#define _WRITER_H

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
}

#include <string>
#include <memory>




class Writer{
public:
    typedef std::shared_ptr<Writer> ptr;
    Writer(AVFormatContext *ofmt_ctx);
    ~Writer();


    int write_header();

    int write_packets(AVPacket *enc_pkt);

    int write_tail();


//    int init_ofmt();


private:
//    std::string filename;
    AVFormatContext *ofmt_ctx;

};






#endif
