#ifndef _ENCODER_H
#define _ENCODER_H

extern "C"{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
#include <libavutil/time.h>
}


#include "writer.h"
#include "timer.h"
#include <string>
#include <memory>


class Encoder{
public:
    typedef std::shared_ptr<Encoder> ptr;
    Encoder(int height = 1080, int width = 1920, int framerate = 30);

    ~Encoder();

    int init_encoder(AVFormatContext *ofmt_ctx);

    int flush();

    int encode(AVFrame *frame, int stream_index, double& count);

    int encode_new(AVFrame *frame, int stream_index, double& count, int64_t& last_time);

    int encode_video(AVFrame* frame, int64_t& last_time);

    int encode_audio(AVFrame* frame);




    int test_encode(AVFrame* frame, int stream_index, double& count, int64_t & time, int& first, int& resend);




private:
   // Writer::ptr writer;
    AVFormatContext *m_ofmt_ctx;

    int height = 1080;
    int width = 1920;
    int framerate = 30;
    AVCodecContext *audio_enc_ctx, *video_enc_ctx;

    double current_time_video = 0.0;
    double current_time_audio = 0.0;

    double video_pts = 0.0;
    double audio_pts = 0.0;

    double time_per_frame_video;
    double time_per_frame_audio;


};




















#endif