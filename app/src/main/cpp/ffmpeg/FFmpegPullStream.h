//
// Created by DELL on 2020/12/3.
//

#ifndef ZHYTOOL_FFMPEGPULLSTREAM_H
#define ZHYTOOL_FFMPEGPULLSTREAM_H
#include <jni.h>
#include <string>

#include <android/log.h>
#include <android/native_window_jni.h>

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
// 为了播放喇叭
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
}
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,"ZPlayer",__VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,"ZPlayer",__VA_ARGS__)
#define MAX_AUDIO_FRAME_SIZE 44100


class FFmpegPullStream {

public:
    int init(JNIEnv *env, jstring url);
    void start(JNIEnv *env);
    void stop(JNIEnv *env);
    void onFrame(JNIEnv *env, jobject play_callback);
    void onDestroy();

protected:

    int videoIndex = -1;
    int audioIndex = -1;
    int count = 0;

    AVFormatContext *pFormatCtx;
    const char *in_filename  = "rtmp://58.200.131.2:1935/livetv/hunantv";   //芒果台rtmp地址
    const char *out_filename_v = "test.h264";   //Output file URL
    int ret = 0;
    bool isstop = false;
    jobject frameCallback = NULL;

    /**
     * 视频相关
     */
    AVPacket *pPacket;
    AVFrame *pAvFrame, *pFrameNv21;
    AVCodecContext *pCodecCtx;
    SwsContext *pImgConvertCtx;
    uint8_t *v_out_buffer;


    /**
     * 音频相关
     */
    AVFrame *aAvFrame;
    AVCodecContext *aCodecCtx;
    SwrContext *aAudioCovertCtx;
    uint8_t *a_out_buffer;
    int out_channer_nb;




private:
    int onDecodeAndRendering();
    void initVideo(JNIEnv *env);
    void initAudio(JNIEnv *env);
    void decodeVideo(JNIEnv *env);
    void decodeAudio(JNIEnv *env);
};


#endif //ZHYTOOL_FFMPEGPULLSTREAM_H
