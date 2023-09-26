#include "utils.h"





AVFrame* Utils::merge_way(int& backround_height, int& backround_width, std::vector<AVFrame *>& frames){
    unsigned int i;
    std::vector<int> heights(frames.size());
    std::vector<int> widths(frames.size());
    std::vector<cv::Mat> imgs(frames.size());


    cv::Mat main_img;
    main_img.create(backround_height, backround_width, CV_8UC3);
    cv::Scalar color(0, 0, 255); // BGR颜色值，这里为绿色
    main_img.setTo(color);

    for(i = 0; i < frames.size(); i++){
        heights[i] = frames[i]->height;
        widths[i] = frames[i]->width;
        imgs[i] = avframe2Mat(heights[i], widths[i], frames[i]);
        cv::resize(imgs[i], imgs[i], cv::Size(backround_width / 2, backround_height / 2));
    }

    for(i = 0; i < frames.size(); i++){
        if(i == 0) add_pic2pic(main_img, imgs[i], 0, 0);
        if(i == 1) add_pic2pic(main_img, imgs[i], main_img.cols / 2, 0);
        if(i == 2) add_pic2pic(main_img, imgs[i], 0, main_img.rows / 2);
        if(i == 3) add_pic2pic(main_img, imgs[i], main_img.cols / 2,  main_img.rows / 2);
    }

//    cv::imshow("main", main_img);
//    cv::waitKey(1);

    AVFrame *dst_frame = Mat2avframe(main_img.cols, main_img.rows, main_img);
    while(!imgs.empty()){
        imgs.pop_back();
    }
    dst_frame->pts = 0;



    return dst_frame;
}


//改变数据帧大小
AVFrame*  Utils::change_frame_size(int new_width, int new_height, AVFrame *src_frame){
    int ret;
    int src_width = src_frame->width;
    int src_height = src_frame->height;
    AVPixelFormat src_fmt = AV_PIX_FMT_YUV420P;


    int dst_width = new_width;
    int dst_height = new_height;
    AVPixelFormat dst_fmt = AV_PIX_FMT_YUV420P;

    AVFrame *dst_frame = av_frame_alloc();
    dst_frame->width = dst_width;
    dst_frame->height = dst_height;
    dst_frame->format = dst_fmt;
    dst_frame->pts = src_frame->pts;

    //alloc space for dst frame
    ret= av_frame_get_buffer(dst_frame, 0);
    if(ret < 0){
        av_log(nullptr, AV_LOG_ERROR, "av_frame_get_buffer failed.\n");
        return nullptr;
    }

    SwsContext *sws_ctx = sws_getContext(src_width, src_height,
                                         src_fmt,
                                         dst_width, dst_height,
                                         dst_fmt,
                                         SWS_FAST_BILINEAR,
                                         nullptr,
                                         nullptr,
                                         nullptr);
    //修改分辨率、数据帧格式的函数
    ret = sws_scale(sws_ctx, src_frame->data, src_frame->linesize, 0, src_height, dst_frame->data, dst_frame->linesize);
    if(ret < 0){
        av_log(nullptr, AV_LOG_ERROR, "sws_scale failed.\n");
        dst_frame = nullptr;
    }

    av_frame_free(&src_frame);
    sws_freeContext(sws_ctx);
    return dst_frame;
}


//avframe转换为Mat
cv::Mat  Utils::avframe2Mat(int& height, int& width, AVFrame *frame){
    int size = height * width;
    cv::Mat img = cv::Mat::zeros(height * 3 / 2, width, CV_8UC1);
    memcpy(img.data,  frame->data[0], size);
    memcpy(img.data + width * height,  frame->data[1], size / 4);
    memcpy(img.data + width * height * 5 / 4, frame->data[2], size / 4);
    //转换图片颜色空间
    cv::cvtColor(img, img, CV_YUV2BGR_I420);
    return img;
}

//Mat转换为avframe
AVFrame*  Utils::Mat2avframe(int& cols, int& rows, cv::Mat img){
    AVFrame *dst_frame = av_frame_alloc();
    dst_frame->height = rows;
    dst_frame->width = cols;
    dst_frame->format = AV_PIX_FMT_YUV420P;
    av_frame_get_buffer(dst_frame, 0);

    cv::cvtColor(img, img, cv::COLOR_BGR2YUV_I420);
    int size = cols * rows;
    unsigned char *data = img.data;
    memcpy(dst_frame->data[0], data, size);
    memcpy(dst_frame->data[1], data + size, size / 4);
    memcpy(dst_frame->data[2], data + size * 5 / 4, size / 4);

    return dst_frame;
}

//叠加图片
void  Utils::add_pic2pic(cv::Mat& src_mat, cv::Mat& added_mat,
                 int col_point, int row_point,
                 double alpha, double beta, double gamma){
    //1. 拿到两个图片的数据
    //2. 选择被添加图片的“感兴趣区域” ROI
    //3. addWeighted函数添加图片
    cv::Mat ROI = src_mat(cv::Rect(col_point, row_point, added_mat.cols, added_mat.rows));
    cv::addWeighted(ROI, alpha, added_mat, beta, gamma, ROI);
}
