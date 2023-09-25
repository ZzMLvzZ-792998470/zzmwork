#ifndef _UTILS_H
#define _UTILS_H


extern "C"{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>
#include <libavutil/pixfmt.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/imgproc/types_c.h>


//合并所有视频帧到一个帧并输出
AVFrame* merge_way(int& backround_height, int& backround_width, std::vector<AVFrame *>& frames);

//改变数据帧画面大小
AVFrame* change_frame_size(int new_height, int new_width, AVFrame *src_frame);

//AVFrame ----> Mat
cv::Mat avframe2Mat(int& height, int& width, AVFrame *frame);

//Mat -----> AVFrame
AVFrame* Mat2avframe(int& cols, int& height, cv::Mat img);

//叠加图片
void add_pic2pic(cv::Mat& src_mat, cv::Mat& added_mat,
                 int col_point = 200, int row_point = 200,
                 double alpha = 0.1, double beta = 0.9, double gamma = 1.0);




#endif
