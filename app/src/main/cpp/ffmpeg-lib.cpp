//
// Created by DELL on 2020/11/25.
//

#include <jni.h>
#include <string>

#include <android/log.h>
#include <android/native_window_jni.h>
#include <thread>
#include <future>

extern "C"
{
#include "libavcodec/avcodec.h"
#include "ffmpeg/AVCodecVideoLocalthread.h"
#include "ffmpeg/AVCodecAudioLocalthread.h"
}
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,"ZPlayer>>",__VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG,"ZPlayer>>",__VA_ARGS__)



/**
 * Class:   ffmpeg
 * Method:    getInfo
 * Signature: (Ljava/lang/String;)V
 */

AVCodecVideoLocalthread *videoLocalthread = nullptr;
AVCodecAudioLocalthread *audioLocalthread = nullptr;

void setVideoPath(char* data){
    if (videoLocalthread) {
        videoLocalthread->setFile(data);
    }
}

void setAudioPath(char* data){
    if (audioLocalthread) {
        audioLocalthread->setFilePath(data);
    }
}

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



extern "C"
JNIEXPORT void JNICALL
Java_com_zhy_zplayer_1ffmpeg_FFmpeg_AVCodecHandler_setAudioCodecFilePath(JNIEnv *env, jobject thiz,
                                                                         jstring file_path,
                                                                         jobject call_back) {


    if (file_path == NULL) {
        return;
    }
    const char* file = env->GetStringUTFChars(file_path, JNI_FALSE);
    char input_str[500] = {0};
    strcpy(input_str,file);
    env->ReleaseStringUTFChars(file_path, file);
    audioLocalthread = new AVCodecAudioLocalthread();
    audioLocalthread->setJniEnv(env);
    audioLocalthread->setAvCallback(call_back);

    std::thread threadAudio(setAudioPath, input_str);
    threadAudio.join();

}

extern "C"
JNIEXPORT void JNICALL
Java_com_zhy_zplayer_1ffmpeg_FFmpeg_AVCodecHandler_setVideoCodecFilePath(JNIEnv *env, jobject thiz,
                                                                         jstring file_path,
                                                                         jobject surface,
                                                                         jobject call_back,
                                                                         jint dst_w, jint dst_h) {
    if (file_path == NULL) {
        return;
    }
    ANativeWindow *window = ANativeWindow_fromSurface(env, surface);
    const char* file = env->GetStringUTFChars(file_path, JNI_FALSE);
    char input_str[500] = {0};
    strcpy(input_str,file);
    env->ReleaseStringUTFChars(file_path, file);
    videoLocalthread = new AVCodecVideoLocalthread();
    videoLocalthread->setJniEnv(env);
    videoLocalthread->setAvCallback(call_back);
    videoLocalthread->setANativeWindow(window);
    videoLocalthread->setDstSize(dst_w, dst_h);

    std::thread threadVideo(setVideoPath, input_str);
    threadVideo.join();
}