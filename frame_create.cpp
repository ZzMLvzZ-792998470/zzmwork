#include "frame_create.h"


FrameCreater::FrameCreater(){


}


FrameCreater::~FrameCreater(){


}


AVFrame *FrameCreater::create_video_frame(int height, int width, int format, int y, int u, int v){
    AVFrame* frame = av_frame_alloc();
    if (!frame) {
        // 处理内存分配失败的情况
        return NULL;
    }

    frame->format = format;
    frame->width = width;
    frame->height = height;

    int ret = av_frame_get_buffer(frame, 0);  // 分配帧数据内存
    if (ret < 0) {
        // 处理内存分配失败的情况
        av_frame_free(&frame);
        return NULL;
    }

    // 设置 Y、U、V 分量的数据指针和行大小
    uint8_t* yData = frame->data[0];
    uint8_t* uData = frame->data[1];
    uint8_t* vData = frame->data[2];
    int yStride = frame->linesize[0];
    int uStride = frame->linesize[1];
    int vStride = frame->linesize[2];

    // 将每个分量的像素值设置为 0（黑色）
    memset(yData, y, yStride * height);
    memset(uData, u, uStride * (height / 2));
    memset(vData, v, vStride * (height / 2));
    return frame;
}

AVFrame *FrameCreater::create_audio_frame( int sample_rate,
                                           int channels,
                                           AVSampleFormat format,
                                           int nb_samples,
                                           int channel_layout){

    AVFrame *frame = av_frame_alloc();

    frame->sample_rate = sample_rate;
    frame->channels = channels;
    frame->format = format;
    frame->nb_samples = nb_samples;
    frame->channel_layout = channel_layout;

    av_frame_get_buffer(frame, 0);

    //设置静音帧
    av_samples_set_silence(frame->data, 0, frame->nb_samples, frame->channels, (AVSampleFormat)frame->format);


    return frame;
}