#include "decoder.h"
#include <iostream>


extern std::mutex mtx;
extern std::condition_variable cond;


Decoder::Decoder(int set_framerate, AVFormatContext *ifmt_ctx) : ifmt_ctx(ifmt_ctx), set_framerate(set_framerate){

}



Decoder::~Decoder() {

}

int Decoder::init_decoder() {
    int ret;
    unsigned int i;

    for (i = 0; i < ifmt_ctx->nb_streams; i++) {
        AVStream *stream = ifmt_ctx->streams[i];
        const AVCodec *dec = avcodec_find_decoder(stream->codecpar->codec_id);
        AVCodecContext *codec_ctx;
        if (!dec) {
            av_log(nullptr, AV_LOG_ERROR, "Failed to find decoder for stream #%u\n", i);
            return AVERROR_DECODER_NOT_FOUND;
        }

        codec_ctx = avcodec_alloc_context3(dec);
        if (!codec_ctx) {
            av_log(nullptr, AV_LOG_ERROR, "Failed to allocate the decoder context for stream #%u\n", i);
            return AVERROR(ENOMEM);
        }
        ret = avcodec_parameters_to_context(codec_ctx, stream->codecpar);
        if (ret < 0) {
            av_log(nullptr, AV_LOG_ERROR, "Failed to copy decoder parameters to input decoder context "
                                          "for stream #%u\n", i);
            return ret;
        }

        //new 非必须
        //codec_ctx->pkt_timebase = stream->time_base;
        /* Reencode video & audio and remux subtitles etc. */
        if (codec_ctx->codec_type == AVMEDIA_TYPE_VIDEO || codec_ctx->codec_type == AVMEDIA_TYPE_AUDIO) {
            if (codec_ctx->codec_type == AVMEDIA_TYPE_VIDEO) {
                codec_ctx->framerate = av_guess_frame_rate(ifmt_ctx, stream, nullptr);
                codec_ctx->bit_rate = ifmt_ctx->bit_rate; //new
            }
            /* Open decoder */ //********************************************* why 1, 48?
            ret = avcodec_open2(codec_ctx, dec, nullptr);
            if (ret < 0) {
                av_log(nullptr, AV_LOG_ERROR, "Failed to open decoder for stream #%u\n", i);
                return ret;
            }
        }

        if(i == 0){
            video_dec_ctx = codec_ctx;
            //video_dec_ctx->time_base = AVRational{1, 24};
            real_framerate = video_dec_ctx->framerate.num;
        }
        else audio_dec_ctx = codec_ctx;
    }
    return 0;

}




int Decoder::decode(int& work) {
    int ret;
    //实际帧率大于设置的帧率，需要丢帧
    if(real_framerate >= set_framerate){
        frameratio = (real_framerate - set_framerate) * 1.0 / set_framerate;
        ret = decode_high2low();
        if(ret < 0){
            av_log(nullptr, AV_LOG_ERROR, "decode_high2low failed.\n");
        }
    //实际帧率小于设置的帧率，需要补帧
    }else{
        frameratio = (set_framerate - real_framerate) * 1.0 / real_framerate;
        ret = decode_low2high();
        if(ret < 0){
            av_log(nullptr, AV_LOG_ERROR, "decode_low2high failed.\n");
        }
    }
    work = 0;
    return ret;
}


int Decoder::decode_high2low() {
    int ret;
    AVPacket *packet = av_packet_alloc();
    while(true) {
        if ((ret = av_read_frame(ifmt_ctx, packet)) < 0) return 0;

        int stream_index = packet->stream_index;

        if(stream_index == 0) ret = avcodec_send_packet(video_dec_ctx, packet);
        else ret = avcodec_send_packet(audio_dec_ctx, packet);
        if (ret < 0) {
            av_log(nullptr, AV_LOG_ERROR, "Decoding failed.\n");
            break;
        }


        av_packet_unref(packet);

        AVFrame *dec_frame = av_frame_alloc();
        while (ret >= 0) {
            if(stream_index == 0) ret = avcodec_receive_frame(video_dec_ctx, dec_frame);
            else ret = avcodec_receive_frame(audio_dec_ctx, dec_frame);

            if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN)) break;
            else if (ret < 0) return ret;


            dec_frame->pts = dec_frame->best_effort_timestamp;
            dec_frame->pict_type = AV_PICTURE_TYPE_NONE;
            //如果是视频帧 需要判断丢帧
            if(stream_index == 0){
                if(count >= 1.0f){
                    count -= 1.0f;
                    //continue;
                }else{
                    count += frameratio;

                    {
                        std::unique_lock<std::mutex> lock(mtx);
                        while(video_queue.size() >= 5){
                            cond.wait(lock);
                        }
                    }


                    {
                        std::lock_guard<std::mutex> lock(mtx);
                        video_queue.push_back(av_frame_clone(dec_frame));
                    }
                }
            } else{


                {
                    std::unique_lock<std::mutex> lock(mtx);
                    while(audio_queue.size() >= 5){
                        cond.wait(lock);
                    }
                }

                {
                    std::lock_guard<std::mutex> lock(mtx);
                    audio_queue.push_back(av_frame_clone(dec_frame));
                }


            }
            av_frame_unref(dec_frame);
        }
        av_frame_free(&dec_frame);
    }
    av_packet_free(&packet);

    return 0;
}


int Decoder::decode_low2high() {
    AVPacket *packet = av_packet_alloc();
    int ret;
    while(true) {
        if (count < 1.0f) {
            if ((ret = av_read_frame(ifmt_ctx, packet)) < 0) {
                return 0;
            }

            int stream_index = packet->stream_index;

            if(stream_index == 0){
                ret = avcodec_send_packet(video_dec_ctx, packet);
                if (ret < 0) {
                    av_log(nullptr, AV_LOG_ERROR, "Decoding failed.\n");
                    break;
                }
            }else{
                ret = avcodec_send_packet(audio_dec_ctx, packet);
                if (ret < 0) {
                    av_log(nullptr, AV_LOG_ERROR, "Decoding failed.\n");
                    break;
                }
            }

            av_packet_unref(packet);
            AVFrame *dec_frame = av_frame_alloc();
            while (ret >= 0) {
                if(stream_index == 0){
                    ret = avcodec_receive_frame(video_dec_ctx, dec_frame);
                }else{
                    ret = avcodec_receive_frame(audio_dec_ctx, dec_frame);
                }
                if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN)) break;
                else if (ret < 0) return ret;


                dec_frame->pts = dec_frame->best_effort_timestamp;
                dec_frame->pict_type = AV_PICTURE_TYPE_NONE;
                if (stream_index == 0) {
                    //写数据加锁
                    {
                        std::unique_lock<std::mutex> lock(mtx);
                        while(video_queue.size() >= 10){
                            cond.wait(lock);
                        }

                    }

                    {
                        std::lock_guard<std::mutex> lock(mtx);
                        video_queue.push_back(av_frame_clone(dec_frame));

                        //内存泄漏存疑点
                        if(count + frameratio >= 1.0f){
                            prev_frame = av_frame_clone(dec_frame);

                        }
                    }
                    av_frame_unref(dec_frame);
                    count += frameratio;
                }else{
                    {
                        std::unique_lock<std::mutex> lock(mtx);
                        while(audio_queue.size() >= 10){
                            cond.wait(lock);
                        }
                    }
                    {
                        std::lock_guard<std::mutex> lock(mtx);
                        audio_queue.push_back(av_frame_clone(dec_frame));
                    }
//                    continue;
                }
            }
            av_frame_free(&dec_frame);
        } else {
            count -= 1.0f;
            {
                std::unique_lock<std::mutex> lock(mtx);
                while(video_queue.size() >= 10){
                    cond.wait(lock);
                }
            }
            {
                std::lock_guard<std::mutex> lock(mtx);
                video_queue.push_back(av_frame_clone(prev_frame));
                if(count < 1.0f) av_frame_free(&prev_frame);
            }
        }
    }
    av_packet_free(&packet);
}
