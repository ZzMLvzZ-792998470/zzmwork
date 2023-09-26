## 大概框架
```angular2html

分类：
                ----> IniterI//初始化ifmt
    Initer-----|
                ----> IniterO//初始化ofmt


    Decoder//解码器类



    Encoder//编码器类


    FrameCreater//帧创建类


    Writer//写文件类


    Transcoder//转码类


创建输入输出文件--->构造Transcoder类--->初始化Initer、Decoder、Encoder

Transcoder--->执行转码操作----》逻辑内部定义

```


## 计时部分
```angular2html

1.编码第1帧的时间--------编码第30帧的时间

2.1帧从提取到编码用了多长时间

3.前一帧编码结束 到新的一帧开始用了多长时间


```
