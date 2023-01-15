//
// Created by 16278 on 2022/5/23.
//

#ifndef ZPLAYER_FFMPEG_AVCODECAUDIOLOCALTHREAD_H
#define ZPLAYER_FFMPEG_AVCODECAUDIOLOCALTHREAD_H
#include "Utils.h"
extern "C"
{
#include "libavformat/avformat.h" //封装格式上下文
#include "libavcodec/avcodec.h" //解码库
#include "libswscale/swscale.h"
#include "libavdevice/avdevice.h"
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

class AVCodecAudioLocalthread {
public:
    AVCodecAudioLocalthread();
    int audioPts = 0;
    int64_t audioTimeStamp = 0;
    int cutAudio = 0;
    void setJniEnv(JNIEnv *env);
    void setAvCallback(jobject call_back);
    void setFilePath(char* path);
    void goPlay(bool play);
    void release();

protected:
    bool play = true;
    //音频流相关
    char *filePath = "";
    jobject avCallback = NULL;
    JNIEnv *env = NULL;
    JavaVM* vm = NULL;
    AVFrame *aAvFrame = nullptr;
    AVPacket *packet = nullptr;
    AVFormatContext *pFormatCtx = nullptr;
    AVCodecContext *aCodecCtx = nullptr;
    SwrContext *aAudioCovertCtx = nullptr;
    uint8_t *a_out_buffer = nullptr;
    int out_channer_nb;
    int audioStream;
    int initAudioCodec();
    int init2AudioCodec();

private:
};


#endif //ZPLAYER_FFMPEG_AVCODECAUDIOLOCALTHREAD_H
