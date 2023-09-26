#include <iostream>

#include "transcoder.h"
//#include <unistd.h>
//
//std::mutex mtx;
//std::condition_variable cond;



void thread_work(Decoder::ptr decoder, int& work){

    std::thread t(&Decoder::decode, decoder, std::ref(work));

    //t.join();
    t.detach();
}



int main() {
    //version1.0


    int ret;
   // std::string file0 = "C://Users//ZZM//Desktop//素材//test.mp4";
    std::string file0 = "C://Users//ZZM//Desktop//素材//网上的素材//抖肩舞.mp4";
    //std::string file0 = "C://Users//ZZM//Desktop//素材//网上的素材//当年陈刀仔.mp4";


    std::string output_file0 = "rtmp://localhost:1935/live/livestream";
    //std::string output_file0 = "output.mp4";

    std::vector<std::string> input_files;
    std::vector<std::string> output_files;

    input_files.push_back(file0);
    //input_files.push_back(file1);

    output_files.push_back(output_file0);
    //output_files.push_back(output_file1);

    Transcoder::ptr t(new Transcoder(input_files, output_files, 1920, 1080, 30));
    t->reencode();

    return 0;

}
