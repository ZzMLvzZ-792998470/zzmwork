#include "encoder.h"
#include <unistd.h>

Encoder::Encoder(int height, int width, int framerate) : height(height), width(width), framerate(framerate){
    //Writer::ptr writer(new Writer());
}


Encoder::~Encoder() {


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

//        if(i == 0){
//            video_enc_ctx = avcodec_alloc_context3(encoder);
//            enc_ctx = video_enc_ctx;
//        }else if(i == 1){
//            audio_enc_ctx = avcodec_alloc_context3(encoder);
//            enc_ctx = audio_enc_ctx;
//        }
        enc_ctx = avcodec_alloc_context3(encoder);

        if (!enc_ctx) {
            av_log(nullptr, AV_LOG_FATAL, "Failed to allocate the encoder context\n");
            return AVERROR(ENOMEM);
        }

        if(enc_ctx->codec_type == AVMEDIA_TYPE_VIDEO){
            enc_ctx->height = height;
            enc_ctx->width = width;
            // enc_ctx->sample_aspect_ratio = dec_ctx->sample_aspect_ratio;
            enc_ctx->gop_size = 1;
            enc_ctx->bit_rate = 4000000;

            /* take first format from list of supported formats */
            //enc_ctx->pix_fmt = dec_ctx->pix_fmt;
            enc_ctx->pix_fmt = AV_PIX_FMT_YUV420P;

            /* video time_base can be set to whatever is handy and supported by encoder */
            enc_ctx->framerate = AVRational {framerate, 1};
            //enc_ctx->time_base =  AVRational {1,framerate}; // {1, framerate}
            enc_ctx->time_base = av_inv_q(enc_ctx->framerate); // {1, framerate}
            video_enc_ctx = enc_ctx;

            time_per_frame_video = 1.0 * 1000 / framerate;

        } else if(enc_ctx->codec_type == AVMEDIA_TYPE_AUDIO){
            enc_ctx->sample_rate = 44100;
            enc_ctx->channel_layout = 3;
            //enc_ctx->sample_rate = dec_ctx->sample_rate;
            //enc_ctx->channel_layout = dec_ctx->channel_layout;
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
            av_interleaved_write_frame(m_ofmt_ctx, enc_pkt);
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

            av_log(nullptr, AV_LOG_INFO, "pts: #%d ", enc_pkt->pts);
            av_log(nullptr, AV_LOG_INFO, " dts: #%d ", enc_pkt->dts);
            av_log(nullptr, AV_LOG_INFO, "audio packet.\n");

            enc_pkt->pts = enc_pkt->dts = (enc_pkt->pts * m_ofmt_ctx->streams[stream_index]->time_base.den / 1000);

            av_interleaved_write_frame(m_ofmt_ctx, enc_pkt);
            //usleep((int)(time_per_frame_audio * 1000));

        }
    }
    av_packet_free(&enc_pkt);
    return 0;
}


int Encoder::flush() {


}


