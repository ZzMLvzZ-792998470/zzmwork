#ifndef _FRAME_CREATE_H
#define _FRAME_CREATE_H

extern "C"{
#include <libavutil/pixdesc.h>
#include <libavutil/pixfmt.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

//#include <opencv2/core/core.hpp>
//#include <opencv2/highgui/highgui.hpp>
//#include <opencv2/imgproc/imgproc.hpp>
//#include <opencv2/imgproc/types_c.h>

#include <memory>



class FrameCreater{
public:
    typedef std::shared_ptr<FrameCreater> ptr;
    FrameCreater();
    ~FrameCreater();


    AVFrame *create_video_frame(int height = 1080, int width = 1920, int format = AV_PIX_FMT_YUV420P, int y = 0, int u = 128, int v = 128);

    AVFrame *create_audio_frame( int sample_rate = 44100,
                                 int channels = 2,
                                 AVSampleFormat format = AV_SAMPLE_FMT_FLTP,
                                 int nb_samples = 1024,
                                 int channel_layout = 3);


private:




};















#endif
