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
    static int write_header(AVFormatContext* ofmt_ctx);

    static int write_packets(AVFormatContext* ofmt_ctx, AVPacket *enc_pkt);

    static int write_tail(AVFormatContext* ofmt_ctx);

private:

};






#endif
