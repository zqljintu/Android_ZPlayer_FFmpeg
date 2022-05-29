//
// Created by 16278 on 2022/5/23.
//

#ifndef ZPLAYER_FFMPEG_UTILS_H
#define ZPLAYER_FFMPEG_UTILS_H
#include <jni.h>
#include <string>
#include <android/log.h>
#include <android/native_window_jni.h>

#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,"ZPlayer",__VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,"ZPlayer",__VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG,"ZPlayer",__VA_ARGS__)
#endif //ZPLAYER_FFMPEG_UTILS_H
