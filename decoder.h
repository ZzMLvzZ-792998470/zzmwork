#ifndef _DECODER_H
#define _DECODER_H


extern "C"{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
}

#include <memory>
#include <deque>
#include <mutex>
#include <condition_variable>
#include <memory>


class Decoder{
public:
    typedef std::shared_ptr<Decoder> ptr;
    Decoder(int set_framerate, AVFormatContext *ifmt_ctx);
    ~Decoder();

    int init_decoder();

    int decode(int& work);

    //补帧解码
    int decode_low2high();

    //丢帧解码
    int decode_high2low();


    std::deque<AVFrame *> get_video_queue(){return video_queue;}

    //解码队列pop操作
    void pop_video(){
        av_frame_free(&video_queue.front());
        video_queue.pop_front();}

    std::deque<AVFrame *> get_audio_queue(){ return audio_queue;}

    void pop_audio(){
        av_frame_free(&audio_queue.front());
        audio_queue.pop_front();
    }



private:
    AVFormatContext *ifmt_ctx;
    int set_framerate;
    int real_framerate;

    AVCodecContext *audio_dec_ctx, *video_dec_ctx;

    double frameratio = 0.0;
    double count = 0.0;

    AVFrame *prev_frame = nullptr;
    std::deque<AVFrame *> video_queue = {};
    std::deque<AVFrame *> audio_queue = {};


};


#endif
