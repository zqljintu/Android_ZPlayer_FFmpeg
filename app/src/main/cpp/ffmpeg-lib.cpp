//
// Created by DELL on 2020/11/25.
//

#include <jni.h>
#include <string>

#include <android/log.h>
#include <android/native_window_jni.h>
extern "C"
{
    #include "libavcodec/avcodec.h"
}
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,"ZPlayer>>",__VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG,"ZPlayer>>",__VA_ARGS__)


/**
 * Class:   ffmpeg
 * Method:    getInfo
 * Signature: (Ljava/lang/String;)V
 */

extern "C"
JNIEXPORT void JNICALL
Java_com_zhy_zplayer_1ffmpeg_FFmpeg_AVCodecHandler_initFFmpeg(JNIEnv *env, jobject thiz) {
    LOGE("ZZZ %s",av_version_info());
}

extern "C"
JNIEXPORT void JNICALL
Java_com_zhy_zplayer_1ffmpeg_FFmpeg_AVCodecHandler_setFilePath(JNIEnv *env, jobject thiz,
                                                               jstring file_path,
                                                               jobject call_back) {

}