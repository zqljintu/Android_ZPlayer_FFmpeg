//
// Created by 16278 on 2022/5/23.
//

#ifndef ZPLAYER_FFMPEG_AVCODECVIDEOLOCALTHREAD_H
#define ZPLAYER_FFMPEG_AVCODECVIDEOLOCALTHREAD_H
#include "Utils.h"

extern "C"
{
#include "libavformat/avformat.h" //封装格式上下文
#include "libavcodec/avcodec.h" //解码库
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"
#include "libavutil/imgutils.h"
#include "libyuv.h"
#include "libavdevice/avdevice.h"
#include "libavutil/channel_layout.h"
#include "libavutil/time.h"
// 为了播放喇叭
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
}

#define MAX_AUDIO_FRAME_SIZE 44100

class AVCodecVideoLocalthread {

public:
    AVCodecVideoLocalthread();
    int64_t videoTimeStamp = 0;
    unsigned int sleepTime = 0;
    void setFile(char* filePath);
    void goPlay(bool play);
    void release();
protected:

private:
    bool play = true;
    char *filePath = "";
    int initVideoCodec();

    int fileTimes = 0;
    int videoStream, i, numBytes;
    int ret, got_picture;
    //视频流相关
    AVFormatContext *pFormatCtx = nullptr;
    AVCodecContext *pCodecCtx = nullptr;
    AVCodec *pCodec = nullptr;
    AVFrame *pFrame = nullptr;
    AVFrame *pFrameRGB = NULL;
    AVPacket *packet = NULL;
    uint8_t *out_buffer = nullptr;
    SwsContext *img_convert_ctx = nullptr;
};


#endif //ZPLAYER_FFMPEG_AVCODECVIDEOLOCALTHREAD_H
