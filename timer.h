#ifndef _TIMER_H
#define _TIMER_H

extern "C"{
#include <libavutil/time.h>
};



class Timer{
public:
    static int64_t getCurrentTime(){
        return av_gettime();
    }


private:



};
















#endif
