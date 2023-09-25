#include "transcoder.h"


std::mutex mtx;
std::condition_variable cond;


Transcoder::Transcoder(std::vector<std::string>& input_filenames,
                       std::vector<std::string> &output_filenames,
                       int width,
                       int height,
                       int framerate) : width(width),
                                        height(height),
                                        framerate(framerate),
                                        inputNums(input_filenames.size()),
                                        outputNums(output_filenames.size()){


    unsigned int i;
    IniterIs.resize(inputNums);
    IniterOs.resize(outputNums);
    decoders.resize(inputNums);
    encoders.resize(outputNums);
    for(i = 0; i < inputNums; i++){
        IniterIs[i] = IniterI::ptr(new IniterI(input_filenames[i]));
        IniterIs[i]->init_fmt();
        decoders[i] = Decoder::ptr(new Decoder(framerate, IniterIs[i]->get_fmt_ctx()));
        decoders[i]->init_decoder();
    }

    for(i = 0; i < outputNums; i++){
        IniterOs[i] = IniterO::ptr(new IniterO(output_filenames[i]));
        IniterOs[i]->init_fmt();

        encoders[i] = Encoder::ptr(new Encoder(height, width, framerate));
        encoders[i]->init_encoder(IniterOs[i]->get_fmt_ctx());

        IniterOs[i]->ofmt_io_open();
        IniterOs[i]->print_ofmt_info();
    }

}


Transcoder::~Transcoder() {

}

void Transcoder::threadwork(int i, int& work){
    std::thread t(&Decoder::decode, decoders[i], std::ref(work));

    //t.join();

    t.detach();
}



int Transcoder::test_encode_as_mp4(int& work){
    //int work = 1;
    double count_video = 0.0;
    double count_audio = 0.0;
    bool all_finish;
    while(true){
        all_finish = (decoders[0]->get_audio_queue().empty() && decoders[0]->get_video_queue().empty() && work == 0);
        if(all_finish) break;

        {
            std::lock_guard<std::mutex> lock(mtx);
            if(!decoders[0]->get_audio_queue().empty()){
                int ret = encoders[0]->encode(av_frame_clone(decoders[0]->get_audio_queue().front()), 1, count_audio);
                if(ret < 0) return -1;
                decoders[0]->pop_audio();
            }

        }

        {
            std::lock_guard<std::mutex> lock(mtx);
            if(!decoders[0]->get_video_queue().empty()){
                int ret = encoders[0]->encode(change_frame_size(1080, 1920, av_frame_clone(decoders[0]->get_video_queue().front())), 0, count_video);
                if(ret < 0) return -1;
                decoders[0]->pop_video();
            }
        }

        {
            std::unique_lock<std::mutex> lock(mtx);
            cond.notify_all();
        }

    }

    encoders[0]->encode(nullptr, 0, count_video);
    encoders[0]->encode(nullptr, 1, count_audio);

    return 0;
}


int Transcoder::test_encode_as_rtmp(int &work) {
    double count_video = 0.0;
    double count_audio = 0.0;
    bool all_finish;


    //while(decoders[0]->get_video_queue().size() < 20 || decoders[0]->get_audio_queue().size() < 20) continue;
    //while(std::abs(decoders[0]->get_video_queue().size() - decoders[0]->get_audio_queue().size()) < 5) continue;
    while(decoders[0]->get_audio_queue().empty() && decoders[0]->get_video_queue().empty()) continue;
    //while(decoders[0]->get_video_queue().size() < 15 && decoders[0]->get_audio_queue().size() < 15) continue;


   //暂时是先后关系 后面可以改成倍率关系
    bool no_empty;
    while(true){
        all_finish = (decoders[0]->get_audio_queue().empty() && decoders[0]->get_video_queue().empty() && work == 0);
        if(all_finish) break;

        {
            std::lock_guard<std::mutex> lock(mtx);
            no_empty = (!decoders[0]->get_video_queue().empty() && !decoders[0]->get_audio_queue().empty());
        }


        if (no_empty) {
            if (count_audio <= count_video) {
                {
                    std::lock_guard<std::mutex> lock(mtx);
                    encoders[0]->encode(av_frame_clone(decoders[0]->get_audio_queue().front()), 1, count_audio);
                    decoders[0]->pop_audio();
                }
            } else {
                {
                    std::lock_guard<std::mutex> lock(mtx);
                    encoders[0]->encode(
                            change_frame_size(1080, 1920, av_frame_clone(decoders[0]->get_video_queue().front())),
                            0, count_video);
                    decoders[0]->pop_video();
                }
            }

            {
                std::unique_lock<std::mutex> lock(mtx);
                cond.notify_all();
            }

        } else {
            if (!decoders[0]->get_audio_queue().empty()) {
                {
                    std::lock_guard<std::mutex> lock(mtx);
                    encoders[0]->encode(av_frame_clone(decoders[0]->get_audio_queue().front()), 1, count_audio);
                    decoders[0]->pop_audio();
                }
            }

            if (!decoders[0]->get_video_queue().empty()) {
                {
                    std::lock_guard<std::mutex> lock(mtx);
                    encoders[0]->encode(change_frame_size(1080, 1920, av_frame_clone(
                            decoders[0]->get_video_queue().front())), 0, count_video);
                    decoders[0]->pop_video();
                }
            }

            {
                std::unique_lock<std::mutex> lock(mtx);
                cond.notify_all();
            }

        }

    }


    encoders[0]->encode(nullptr, 0, count_video);
    encoders[0]->encode(nullptr, 1, count_audio);

    return 0;

}


int Transcoder::test_encode_as_rtmp2(int &work) {
    double count_video = 0.0;
    double count_audio = 0.0;
    bool all_finish;


    FrameCreater::ptr c(new FrameCreater());
    AVFrame *temp = c->create_video_frame();

    AVFrame* last_frame = temp;


    while(true){
        all_finish = work == 0 && decoders[0]->get_video_queue().empty() && decoders[0]->get_audio_queue().empty();
        if(all_finish) break;


        //如果包都有的话-----》
        if(count_audio <= count_video){
            if(!decoders[0]->get_audio_queue().empty()){
                {
                    std::lock_guard<std::mutex> lock(mtx);
                    encoders[0]->encode(av_frame_clone(decoders[0]->get_audio_queue().front()), 1, count_audio);
                    decoders[0]->pop_audio();
                }
            }
        } else{
            if(!decoders[0]->get_video_queue().empty()){
                {
                    std::lock_guard<std::mutex> lock(mtx);
                    encoders[0]->encode(
                            change_frame_size(1080, 1920, av_frame_clone(decoders[0]->get_video_queue().front())),
                            0, count_video);

                    av_frame_free(&last_frame);
                    last_frame = av_frame_clone(decoders[0]->get_video_queue().front());
                    decoders[0]->pop_video();
                }
            } else{
                encoders[0]->encode(change_frame_size(1080, 1920, av_frame_clone(last_frame)), 0, count_video);
            }
        }

        {
            std::unique_lock<std::mutex> lock(mtx);
            cond.notify_all();
        }

        if(work == 0 && (decoders[0]->get_audio_queue().empty() || decoders[0]->get_video_queue().empty())){
            while(!decoders[0]->get_audio_queue().empty()){
                {
                    std::lock_guard<std::mutex> lock(mtx);
                    encoders[0]->encode(av_frame_clone(decoders[0]->get_audio_queue().front()), 1, count_audio);
                    decoders[0]->pop_audio();
                }
            }


            while(!decoders[0]->get_video_queue().empty()){
                {
                    std::lock_guard<std::mutex> lock(mtx);
                    encoders[0]->encode(
                            change_frame_size(1080, 1920, av_frame_clone(decoders[0]->get_video_queue().front())),
                            0, count_video);
                    decoders[0]->pop_video();
                }
            }
        }
    }




    encoders[0]->encode(nullptr, 0, count_video);
    encoders[0]->encode(nullptr, 1, count_audio);




}



int Transcoder::reencode() {
    int work = 1;
    unsigned int i;

    Writer::ptr w1(new Writer(IniterOs[0]->get_fmt_ctx()));

    w1->write_header();
    for(i = 0; i < inputNums; i++){
        threadwork(i, work);
    }

    //test_encode_as_mp4(work);
    test_encode_as_rtmp2(work);

    w1->write_tail();

    return 0;
}




