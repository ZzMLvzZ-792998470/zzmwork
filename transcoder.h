#ifndef _TRANSCODER_H
#define _TRANSCODER_H

#include "initer.h"
#include "decoder.h"
#include "encoder.h"
#include "writer.h"
#include "frame_create.h"
#include "utils.h"
#include "timer.h"

#include <thread>
#include <mutex>
#include <condition_variable>


class Transcoder{
public:
    typedef std::shared_ptr<Transcoder> ptr;
    Transcoder(std::vector<std::string>& input_filenames,
               std::vector<std::string>& output_filenames,
               int width = 1920,
               int height = 1080,
               int framerate = 30);
    ~Transcoder();

    void threadwork(int i, int& work);

    int reencode();

    int encode_as_mp4(int& work);
    int encode_as_rtmp(int& work);

    int test_encode_rtmp4(int& work);


private:
    int width;
    int height;

    int framerate;

    int inputNums;
    int outputNums;

    std::vector<IniterI::ptr> IniterIs;
    std::vector<IniterO::ptr> IniterOs;

    std::vector<Decoder::ptr> decoders;
    std::vector<Encoder::ptr> encoders;

};















#endif
