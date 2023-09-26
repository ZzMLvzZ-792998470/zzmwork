#include "encoder.h"
#include <unistd.h>

Encoder::Encoder(int height, int width, int framerate) : height(height), width(width), framerate(framerate){

}


Encoder::~Encoder() {
    m_ofmt_ctx = nullptr;
    avcodec_free_context(&audio_enc_ctx);
    avcodec_free_context(&video_enc_ctx);
}



int Encoder::init_encoder(AVFormatContext *ofmt_ctx) {
    int ret;
    unsigned int i;
    AVCodecContext *enc_ctx;

    for (i = 0; i < 2; i++) {
        AVStream *out_stream = avformat_new_stream(ofmt_ctx,  nullptr);
        if (!out_stream) {
            av_log(nullptr, AV_LOG_ERROR, "Failed allocating output stream\n");
            return AVERROR_UNKNOWN;
        }

        const AVCodec *encoder = i == 0 ? avcodec_find_encoder_by_name("libx264") : avcodec_find_encoder(AV_CODEC_ID_AAC);
        //const AVCodec *encoder = i == 0 ? avcodec_find_encoder_by_name("h264_nvenc") : avcodec_find_encoder(AV_CODEC_ID_AAC);
        if(!encoder){
            av_log(nullptr, AV_LOG_FATAL, "Necessary encoder not found\n");
            return AVERROR_INVALIDDATA;
        }

        enc_ctx = avcodec_alloc_context3(encoder);
        if (!enc_ctx) {
            av_log(nullptr, AV_LOG_FATAL, "Failed to allocate the encoder context\n");
            return AVERROR(ENOMEM);
        }

        if(enc_ctx->codec_type == AVMEDIA_TYPE_VIDEO){
            enc_ctx->height = height;
            enc_ctx->width = width;

//            enc_ctx->bit_rate = 800000;
//            enc_ctx->bit_rate_tolerance = 800000;
//            enc_ctx->max_b_frames = 0;
//            enc_ctx->rc_min_rate = 800000;
//            enc_ctx->rc_max_rate = 800000;
//            enc_ctx->rc_buffer_size = enc_ctx->bit_rate;
//            enc_ctx->rc_initial_buffer_occupancy = enc_ctx->rc_buffer_size*3 / 4;


           // enc_ctx->gop_size = 24;
            //enc_ctx->global_quality = 50;
            // enc_ctx->sample_aspect_ratio = dec_ctx->sample_aspect_ratio;
//            enc_ctx->gop_size = 12;
//            enc_ctx->bit_rate = 4000000;

            /* take first format from list of supported formats */
            //enc_ctx->pix_fmt = dec_ctx->pix_fmt;
            enc_ctx->pix_fmt = AV_PIX_FMT_YUV420P;

            /* video time_base can be set to whatever is handy and supported by encoder */
            enc_ctx->framerate = AVRational {framerate, 1};
            enc_ctx->time_base = av_inv_q(enc_ctx->framerate); // {1, framerate}
            video_enc_ctx = enc_ctx;


//            "veryfast": 比 "superfast" 稍慢一些，提供更好的画质。
//            "faster": 比 "veryfast" 稍慢一些，进一步提升画质。
//            "fast": 比 "faster" 稍慢一些，综合了速度和画质。
//            "medium": 中等速度和画质的预设，提供了较好的平衡。
//            "slow": 比 "medium" 慢一些，提供更高的画质。
//            "slower": 比 "slow" 更慢，进一步提升画质。
//            "veryslow": 最慢的预设，提供最高质量的编码结果


            av_opt_set(video_enc_ctx->priv_data, "preset", "ultrafast", 0);
            time_per_frame_video = 1.0 * 1000 / framerate;

        } else if(enc_ctx->codec_type == AVMEDIA_TYPE_AUDIO){
            enc_ctx->sample_rate = 44100;
            enc_ctx->channel_layout = 3;
            enc_ctx->channels = av_get_channel_layout_nb_channels(enc_ctx->channel_layout);

            /* take first format from list of supported formats */
            enc_ctx->sample_fmt = encoder->sample_fmts[0];
            enc_ctx->time_base = AVRational{1, enc_ctx->sample_rate};
            //enc_ctx->time_base = AVRational{1024, enc_ctx->sample_rate};
            audio_enc_ctx = enc_ctx;
            time_per_frame_audio = 1024 * 1.0 * 1000 / audio_enc_ctx->sample_rate;
        }


        if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
            enc_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

        ret = avcodec_open2(enc_ctx, encoder, nullptr);
        if (ret < 0) {
            av_log(nullptr, AV_LOG_ERROR, "Cannot open video encoder for stream #%u\n", i);
            return ret;
        }


        ret = avcodec_parameters_from_context(out_stream->codecpar, enc_ctx);
        if (ret < 0) {
            av_log(nullptr, AV_LOG_ERROR, "Failed to copy encoder parameters to output stream #%u\n", i);
            return ret;
        }
    }

    m_ofmt_ctx = ofmt_ctx;
    return 0;
}



int Encoder::encode(AVFrame *frame, int stream_index, double& count) {
    int ret;
    AVFrame *encode_frame = frame;
    AVPacket *enc_pkt = av_packet_alloc();

    if(stream_index == 0){
        //设置编码数据帧的pts --->{1, 25} 表示 一个时间单位一帧 正确
        if(encode_frame) encode_frame->pts = video_pts;
        video_pts++;

        ret = avcodec_send_frame(video_enc_ctx, encode_frame);  //耗时最长
        if(ret < 0){
            av_log(nullptr, AV_LOG_ERROR, "video_frame: avcodec_send_frame failed.\n");
            return ret;
        }
        av_frame_free(&frame);

        while (ret >= 0) {
            ret = avcodec_receive_packet(video_enc_ctx, enc_pkt);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;


            enc_pkt->stream_index = stream_index;

            //33.333ms.... --->一帧要多少毫秒
            enc_pkt->dts = enc_pkt->pts = current_time_video;
            current_time_video += time_per_frame_video;
            count += time_per_frame_video;

            av_log(nullptr, AV_LOG_INFO, "pts: #%d ", enc_pkt->pts);
            av_log(nullptr, AV_LOG_INFO, " dts: #%d ", enc_pkt->dts);
            av_log(nullptr, AV_LOG_INFO, "video packet.\n");

            enc_pkt->pts = enc_pkt->dts = (enc_pkt->pts * m_ofmt_ctx->streams[stream_index]->time_base.den / 1000);
            Writer::write_packets(m_ofmt_ctx, enc_pkt);
        }

    }else{
        //设置编码数据帧的pts --->{1, 44100} 表示 一个时间单位一帧 正确
        if(encode_frame) encode_frame->pts = audio_pts;
        audio_pts += 1024;

        ret = avcodec_send_frame(audio_enc_ctx, encode_frame);
        if(ret < 0){
            av_log(nullptr, AV_LOG_ERROR, "audio_frame: avcodec_send_frame failed.\n");
            return ret;
        }

        av_frame_free(&encode_frame);
        while (ret >= 0) {
            ret = avcodec_receive_packet(audio_enc_ctx, enc_pkt);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;


            enc_pkt->stream_index = stream_index;

            //23.219ms.... --->一帧要多少毫秒
            enc_pkt->dts = enc_pkt->pts = current_time_audio;
            current_time_audio += time_per_frame_audio;
            count += time_per_frame_audio;

            av_log(nullptr, AV_LOG_INFO, "pts: #%d ", enc_pkt->pts);
            av_log(nullptr, AV_LOG_INFO, " dts: #%d ", enc_pkt->dts);
            av_log(nullptr, AV_LOG_INFO, "audio packet.\n");

            enc_pkt->pts = enc_pkt->dts = (enc_pkt->pts * m_ofmt_ctx->streams[stream_index]->time_base.den / 1000);

            Writer::write_packets(m_ofmt_ctx, enc_pkt);
        }
    }
    av_packet_free(&enc_pkt);
    return 0;
}

int Encoder::encode_new(AVFrame *frame, int stream_index, double &count, int64_t &last_time) {
    int64_t timeEncode = Timer::getCurrentTime();
    int ret;
    AVFrame *encode_frame = frame;
    AVPacket *enc_pkt = av_packet_alloc();

    if(stream_index == 0) {
        //设置编码数据帧的pts --->{1, 25} 表示 一个时间单位一帧 正确
        if (encode_frame) encode_frame->pts = video_pts;
        video_pts++;

        ret = avcodec_send_frame(video_enc_ctx, encode_frame);  //耗时最长
        if (ret < 0) {
            av_log(nullptr, AV_LOG_ERROR, "video_frame: avcodec_send_frame failed.\n");
            return ret;
        }

        av_frame_free(&frame);

        while (ret >= 0) {
            ret = avcodec_receive_packet(video_enc_ctx, enc_pkt);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                break;
            }

            enc_pkt->stream_index = stream_index;

            //33.333ms.... --->一帧要多少毫秒
            enc_pkt->dts = enc_pkt->pts = current_time_video;
            current_time_video += time_per_frame_video;
            count += time_per_frame_video;

//            av_log(nullptr, AV_LOG_INFO, "pts: #%d ", enc_pkt->pts);
//            av_log(nullptr, AV_LOG_INFO, " dts: #%d ", enc_pkt->dts);
//            av_log(nullptr, AV_LOG_INFO, "video packet.\n");

            enc_pkt->pts = enc_pkt->dts = (enc_pkt->pts * m_ofmt_ctx->streams[stream_index]->time_base.den / 1000);
            int64_t sep = 0;
            //Writer::write_packets(m_ofmt_ctx, enc_pkt);

            av_interleaved_write_frame(m_ofmt_ctx, enc_pkt);


            av_log(nullptr, AV_LOG_INFO, "push_success use time: %dms.\n", (Timer::getCurrentTime() - timeEncode) / 1000);
            if(last_time != -1) {
                int64_t sleep_time = (1000 / framerate * 1000 + 10000 - 12500 - (Timer::getCurrentTime() - last_time));
                if(sleep_time > 0){
                    av_usleep(sleep_time);
                }
            }
            last_time = Timer::getCurrentTime();
        }

    }else{
        //设置编码数据帧的pts --->{1, 44100} 表示 一个时间单位一帧 正确
        if(encode_frame) encode_frame->pts = audio_pts;
        audio_pts += 1024;
        //count+=1024;

        ret = avcodec_send_frame(audio_enc_ctx, encode_frame);
        if(ret < 0){
            av_log(nullptr, AV_LOG_ERROR, "audio_frame: avcodec_send_frame failed.\n");
            return ret;
        }

        av_frame_free(&encode_frame);

        while (ret >= 0) {
            ret = avcodec_receive_packet(audio_enc_ctx, enc_pkt);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF){
                //return 0;
                break;
            }

            enc_pkt->stream_index = stream_index;

            //23.219ms.... --->一帧要多少毫秒
            enc_pkt->dts = enc_pkt->pts = current_time_audio;
            current_time_audio += time_per_frame_audio;
            count += time_per_frame_audio;

//            av_log(nullptr, AV_LOG_INFO, "pts: #%d ", enc_pkt->pts);
//            av_log(nullptr, AV_LOG_INFO, " dts: #%d ", enc_pkt->dts);
//            av_log(nullptr, AV_LOG_INFO, "audio packet.\n");

            enc_pkt->pts = enc_pkt->dts = (enc_pkt->pts * m_ofmt_ctx->streams[stream_index]->time_base.den / 1000);

            //Writer::write_packets(m_ofmt_ctx, enc_pkt);
            av_interleaved_write_frame(m_ofmt_ctx, enc_pkt);


        }
    }
    av_packet_free(&enc_pkt);
    if(stream_index == 0) av_log(nullptr, AV_LOG_INFO, "Whole func use time: %dms.\n", (Timer::getCurrentTime() - timeEncode) / 1000);
    //if(stream_index == 0) last_time = Timer::getCurrentTime();
    return 0;
}


int Encoder::encode_video(AVFrame *frame, int64_t &last_time) {
    int64_t video_encode_start_time = Timer::getCurrentTime();
    int ret;
    int stream_index = 0;
    AVFrame *encode_frame = frame;
    AVPacket *enc_pkt = av_packet_alloc();


    //设置编码数据帧的pts --->{1, framerate} 表示 一个时间单位一帧 正确
    if (encode_frame) encode_frame->pts = (int64_t)video_pts;
    video_pts++;

    ret = avcodec_send_frame(video_enc_ctx, encode_frame);
    if (ret < 0) {
        av_log(nullptr, AV_LOG_ERROR, "video_frame: avcodec_send_frame failed.\n");
        return ret;
    }

    av_frame_free(&frame);
    while (ret >= 0) {
        ret = avcodec_receive_packet(video_enc_ctx, enc_pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;

        enc_pkt->stream_index = stream_index;

        //33.333ms.... --->一帧要多少毫秒
        enc_pkt->dts = enc_pkt->pts = current_time_video;
        current_time_video += time_per_frame_video;

        av_log(nullptr, AV_LOG_INFO, "pts: #%d ", enc_pkt->pts);
        av_log(nullptr, AV_LOG_INFO, " dts: #%d ", enc_pkt->dts);
        av_log(nullptr, AV_LOG_INFO, "video packet.\n");

        enc_pkt->pts = enc_pkt->dts = (enc_pkt->pts * m_ofmt_ctx->streams[stream_index]->time_base.den / 1000);
        Writer::write_packets(m_ofmt_ctx, enc_pkt);

        //av_interleaved_write_frame(m_ofmt_ctx, enc_pkt);

       // av_log(nullptr, AV_LOG_INFO, "video start------>push_success time: %dms.\n", (Timer::getCurrentTime() - video_encode_start_time) / 1000);
        if(last_time != -1) {
            int64_t sleep_time = (1000 / framerate * 1000 + 10000 - 12500 - (Timer::getCurrentTime() - last_time));
            if(sleep_time > 0){
                av_usleep(sleep_time);
            }
        }
        last_time = Timer::getCurrentTime();
    }

    av_packet_free(&enc_pkt);
    av_log(nullptr, AV_LOG_INFO, "Whole video encode func time: %dms.\n", (Timer::getCurrentTime() - video_encode_start_time) / 1000);
    return 0;
}



int Encoder::encode_audio(AVFrame *frame) {
    int64_t audio_encode_start_time = Timer::getCurrentTime();
    int ret;
    int stream_index = 1;
    AVFrame *encode_frame = frame;
    AVPacket *enc_pkt = av_packet_alloc();

    //设置编码数据帧的pts --->{1, 44100} 表示 一个时间单位一帧 正确
    if(encode_frame) encode_frame->pts = audio_pts;
    audio_pts += 1024;

    ret = avcodec_send_frame(audio_enc_ctx, encode_frame);
    if(ret < 0){
        av_log(nullptr, AV_LOG_ERROR, "audio_frame: avcodec_send_frame failed.\n");
        return ret;
    }

    av_frame_free(&encode_frame);
    while (ret >= 0) {
        ret = avcodec_receive_packet(audio_enc_ctx, enc_pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;


        enc_pkt->stream_index = stream_index;

        //23.219ms.... --->一帧要多少毫秒
        enc_pkt->dts = enc_pkt->pts = current_time_audio;
        current_time_audio += time_per_frame_audio;


        av_log(nullptr, AV_LOG_INFO, "pts: #%d ", enc_pkt->pts);
        av_log(nullptr, AV_LOG_INFO, " dts: #%d ", enc_pkt->dts);
        av_log(nullptr, AV_LOG_INFO, "audio packet.\n");

        enc_pkt->pts = enc_pkt->dts = (enc_pkt->pts * m_ofmt_ctx->streams[stream_index]->time_base.den / 1000);

        Writer::write_packets(m_ofmt_ctx, enc_pkt);
       // av_interleaved_write_frame(m_ofmt_ctx, enc_pkt);

    }

    av_packet_free(&enc_pkt);
    av_log(nullptr, AV_LOG_INFO, "Whole audio encode func time: %dms.\n", (Timer::getCurrentTime() - audio_encode_start_time) / 1000);
    return 0;
}


int Encoder::test_encode(AVFrame *frame, int stream_index, double &count, int64_t & time, int& first, int& resend) {
    int ret;
    AVFrame *encode_frame = frame;
    AVPacket *enc_pkt = av_packet_alloc();

    if(stream_index == 0){
        //设置编码数据帧的pts --->{1, 25} 表示 一个时间单位一帧 正确
        if(encode_frame) encode_frame->pts = video_pts;
        video_pts++;

        ret = avcodec_send_frame(video_enc_ctx, encode_frame);
        if(ret < 0){
            av_log(nullptr, AV_LOG_ERROR, "video_frame: avcodec_send_frame failed.\n");
            return ret;
        }

        av_frame_free(&encode_frame);

        while (ret >= 0) {
            ret = avcodec_receive_packet(video_enc_ctx, enc_pkt);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) return 0;

            enc_pkt->stream_index = stream_index;

            //33.333ms.... --->一帧要多少毫秒
            enc_pkt->dts = enc_pkt->pts = current_time_video;
            current_time_video += time_per_frame_video;
            count += time_per_frame_video;

            av_log(nullptr, AV_LOG_INFO, "pts: #%d ", enc_pkt->pts);
            av_log(nullptr, AV_LOG_INFO, " dts: #%d ", enc_pkt->dts);
            av_log(nullptr, AV_LOG_INFO, "video packet.\n");

            enc_pkt->pts = enc_pkt->dts = (enc_pkt->pts * m_ofmt_ctx->streams[stream_index]->time_base.den / 1000);
            //Writer::write_packets(m_ofmt_ctx, enc_pkt);
            //int64_t  time1 = Timer::getCurrentTime();
            av_interleaved_write_frame(m_ofmt_ctx, enc_pkt);
           // av_log(nullptr, AV_LOG_INFO, "time1: %dms.\n", (Timer::getCurrentTime() - time1) / 1000);

            //av_log(nullptr, AV_LOG_INFO, "2 video packets seperate: %dms.\n", (Timer::getCurrentTime() - time) / 1000);
            if(first == 1){
                time = Timer::getCurrentTime();
                first = 0;
            } else{
//                int sep =  (Timer::getCurrentTime() - time) / 1000;
//                if(sep < 33){
//                    resend -= (33 / sep);
//                }
//
//
//                av_log(nullptr, AV_LOG_INFO, "2 video packets seperate: %dms.\n", sep);
//                time = Timer::getCurrentTime();
//                resend = sep >= 50 ? sep / 33 : resend;
             //   av_log(nullptr, AV_LOG_INFO, "2 video packets seperate: %dms.\n", (Timer::getCurrentTime() - time) / 1000);
                time = Timer::getCurrentTime();
            }


            //av_usleep(10000);
            //usleep((int)(time_per_frame_video * 1000));
        }

    }else{
        //设置编码数据帧的pts --->{1, 44100} 表示 一个时间单位一帧 正确
        if(encode_frame) encode_frame->pts = audio_pts;
        audio_pts += 1024;

        ret = avcodec_send_frame(audio_enc_ctx, encode_frame);
        if(ret < 0){
            av_log(nullptr, AV_LOG_ERROR, "audio_frame: avcodec_send_frame failed.\n");
            return ret;
        }

        av_frame_free(&encode_frame);

        while (ret >= 0) {
            ret = avcodec_receive_packet(audio_enc_ctx, enc_pkt);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) return 0;

            enc_pkt->stream_index = stream_index;

            //23.219ms.... --->一帧要多少毫秒
            enc_pkt->dts = enc_pkt->pts = current_time_audio;
            current_time_audio += time_per_frame_audio;
            count += time_per_frame_audio;

//            av_log(nullptr, AV_LOG_INFO, "pts: #%d ", enc_pkt->pts);
//            av_log(nullptr, AV_LOG_INFO, " dts: #%d ", enc_pkt->dts);
//            av_log(nullptr, AV_LOG_INFO, "audio packet.\n");

            enc_pkt->pts = enc_pkt->dts = (enc_pkt->pts * m_ofmt_ctx->streams[stream_index]->time_base.den / 1000);

            //Writer::write_packets(m_ofmt_ctx, enc_pkt);

            av_interleaved_write_frame(m_ofmt_ctx, enc_pkt);
            if(first == 1){
                time = Timer::getCurrentTime();
                first = 0;
            } else{
                int sep =  (Timer::getCurrentTime() - time) / 1000;
                av_log(nullptr, AV_LOG_INFO, "2 audio packets seperate: %dms.\n", sep);
                time = Timer::getCurrentTime();

                //resend = sep >= 50 ? 1 : 0;
            }

            //av_usleep(time_per_frame_audio * 1000);
            //usleep((int)(time_per_frame_audio * 1000));

        }
    }
    av_packet_free(&enc_pkt);
    return 0;
}


int Encoder::flush() {


}


