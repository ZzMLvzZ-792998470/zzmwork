#ifndef _INITER_H
#define _INITER_H

extern "C"{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
}

#include <string>
#include <memory>



class Initer{
public:
    typedef std::shared_ptr<Initer> ptr;

    Initer(){};
    virtual ~Initer() = 0;

    virtual int init_fmt() = 0;

    virtual AVFormatContext* get_fmt_ctx() = 0;


private:
    std::string filename;
    AVFormatContext *fmt_ctx;

};


class IniterI : Initer{
public:
    typedef std::shared_ptr<IniterI> ptr;
    IniterI(const std::string& filename);
    ~IniterI() override;

    int init_fmt() override;
    AVFormatContext* get_fmt_ctx() override;

private:
    std::string filename;
    AVFormatContext *ifmt_ctx = nullptr;

};




class IniterO : Initer{
public:
    typedef std::shared_ptr<IniterO> ptr;
    IniterO(const std::string& filename);
    ~IniterO() override;


    int init_fmt() override;
    AVFormatContext* get_fmt_ctx() override;

    void print_ofmt_info();

    int ofmt_io_open();


private:
    std::string filename;
    AVFormatContext *ofmt_ctx = nullptr;

};


#endif
