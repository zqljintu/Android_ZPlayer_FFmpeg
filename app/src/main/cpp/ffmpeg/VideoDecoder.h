//
// Created by 16278 on 2022/5/28.
//

#ifndef ZPLAYER_FFMPEG_VIDEODECODER_H
#define ZPLAYER_FFMPEG_VIDEODECODER_H
#include "Utils.h"
#include <jni.h>
#include <mutex>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <time.h>

extern "C" {
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libswresample/swresample.h"
#include "libswscale/swscale.h"
}
#include <string>


class VideoDecoder {
private:
    std::string path_;
    long time_ms_ = -1;
    long last_decode_time_ms_ = -1;
    bool is_seeking_ = false;
    ANativeWindow* native_window_ = nullptr;
    ANativeWindow_Buffer window_buffer_{};
    // 视频宽高属性
    int video_width_ = 0;
    int video_height_ = 0;
    uint8_t* out_buffer_ = nullptr;
    // on_decode_frame 用于将抽取指定帧的时间戳回调给上层解码器，以供上层解码器进行其他操作。
    std::function<void(long timestamp)> on_decode_frame_ = nullptr;
    bool is_stop_ = false;

    // 会与在循环同步时用的锁 “std::unique_lock<std::mutex>” 配合使用
    std::mutex work_queue_mtx;
    // 真正在进行同步等待和唤醒的属性
    std::condition_variable condition_;
    // 解码器真正进行解码的函数
    void Decode(AVCodecContext* codec_ctx, AVPacket* pkt, AVFrame* frame, AVStream* stream,
                std::unique_lock<std::mutex>& lock, SwsContext* sws_context, AVFrame* pFrame);

public:
    // 新建解码器时要传入媒体文件路径和一个解码后的回调 on_decode_frame。
    VideoDecoder(const char* path, std::function<void(long timestamp)> on_decode_frame);
    // 在 JNI 层将上层传入的 Surface 包装后新建一个 ANativeWindow 传入，在后面解码后绘制帧数据时需要用到
    void Prepare(ANativeWindow* window);
    // 抽取指定时间戳的视频帧，可由上层调用
    bool DecodeFrame(long time_ms);
    // 释放解码器资源
    void Release();
    // 获取当前系统毫秒时间
    static int64_t GetCurrentMilliTime(void);
};


#endif //ZPLAYER_FFMPEG_VIDEODECODER_H
