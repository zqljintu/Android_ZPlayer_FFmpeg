//
// Created by 16278 on 2022/5/28.
//

#include "VideoDecoder.h"
#include <thread>
#include <utility>
#define TAG "VideoDecoder"
extern "C" {
#include <libavutil/imgutils.h>
}

VideoDecoder::VideoDecoder(const char* path, std::function<void(long timestamp)> on_decode_frame)
        : on_decode_frame_(std::move(on_decode_frame)) {
    path_ = std::string(path);
}

void VideoDecoder::Decode(AVCodecContext* codec_ctx, AVPacket* pkt, AVFrame* frame, AVStream* stream,
                          std::unique_lock<std::mutex>& lock, SwsContext* sws_context,
                          AVFrame* rgba_frame) {

    int ret;
    /* send the packet with the compressed data to the decoder */
    ret = avcodec_send_packet(codec_ctx, pkt);
    if (ret == AVERROR(EAGAIN)) {
        LOGE(TAG,
             "Decode: Receive_frame and send_packet both returned EAGAIN, which is an API violation.");
    } else if (ret < 0) {
        return;
    }

    // read all the output frames (infile general there may be any number of them
    while (ret >= 0 && !is_stop_) {
        // 对于frame, avcodec_receive_frame内部每次都先调用
        ret = avcodec_receive_frame(codec_ctx, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            return;
        } else if (ret < 0) {
            return;
        }
        int64_t startTime = GetCurrentMilliTime();
        LOGD(TAG, "decodeStartTime: %ld", startTime);
        // 换算当前解码的frame时间戳
        auto decode_time_ms = frame->pts * 1000 / stream->time_base.den;
        LOGD(TAG, "decode_time_ms = %ld", decode_time_ms);
        if (decode_time_ms >= time_ms_) {
            LOGD(TAG, "decode decode_time_ms = %ld, time_ms_ = %ld", decode_time_ms, time_ms_);
            last_decode_time_ms_ = decode_time_ms;
            is_seeking_ = false;

            // 数据格式转换
            int result = sws_scale(
                    sws_context,
                    (const uint8_t* const*) frame->data, frame->linesize,
                    0, video_height_,
                    rgba_frame->data, rgba_frame->linesize);

            if (result <= 0) {
                LOGE(TAG, "Player Error : data convert fail");
                return;
            }

            // 播放
            result = ANativeWindow_lock(native_window_, &window_buffer_, nullptr);
            if (result < 0) {
                LOGE(TAG, "Player Error : Can not lock native window");
            } else {
                // 将图像绘制到界面上
                auto bits = (uint8_t*) window_buffer_.bits;
                for (int h = 0; h < video_height_; h++) {
                    memcpy(bits + h * window_buffer_.stride * 4,
                           out_buffer_ + h * rgba_frame->linesize[0],
                           rgba_frame->linesize[0]);
                }
                ANativeWindow_unlockAndPost(native_window_);
            }
            on_decode_frame_(decode_time_ms);
            int64_t endTime = GetCurrentMilliTime();
            LOGD(TAG, "decodeEndTime - decodeStartTime: %ld", endTime - startTime);
            LOGD(TAG, "finish decode frame");
            condition_.wait(lock);
        }
        // 主要作用是清理AVPacket中的所有空间数据，清理完毕后进行初始化操作，并且将 data 与 size 置为0，方便下次调用。
        // 释放 packet 引用
        av_packet_unref(pkt);
    }
}

void VideoDecoder::Prepare(ANativeWindow* window) {
    native_window_ = window;
    av_register_all();
    auto av_format_context = avformat_alloc_context();
    avformat_open_input(&av_format_context, path_.c_str(), nullptr, nullptr);
    avformat_find_stream_info(av_format_context, nullptr);
    int video_stream_index = -1;
    for (int i = 0; i < av_format_context->nb_streams; i++) {
        // 找到视频媒体流的下标
        if (av_format_context->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_index = i;
            LOGD(TAG, "find video stream index = %d", video_stream_index);
            break;
        }
    }

    // run once
    do {
        if (video_stream_index == -1) {
            LOGE(TAG, "Player Error : Can not find video stream");
            break;
        }
        std::unique_lock<std::mutex> lock(work_queue_mtx);

        // 获取视频媒体流
        auto stream = av_format_context->streams[video_stream_index];
        // 找到已注册的解码器
        auto codec = avcodec_find_decoder(stream->codecpar->codec_id);
        // 获取解码器上下文
        AVCodecContext* codec_ctx = avcodec_alloc_context3(codec);
        auto ret = avcodec_parameters_to_context(codec_ctx, stream->codecpar);

        if (ret >= 0) {
            // 打开
            avcodec_open2(codec_ctx, codec, nullptr);
            // 解码器打开后才有宽高的值
            video_width_ = codec_ctx->width;
            video_height_ = codec_ctx->height;

            AVFrame* rgba_frame = av_frame_alloc();
            int buffer_size = av_image_get_buffer_size(AV_PIX_FMT_RGBA, video_width_, video_height_,
                                                       1);
            // 分配内存空间给输出 buffer
            out_buffer_ = (uint8_t*) av_malloc(buffer_size * sizeof(uint8_t));
            av_image_fill_arrays(rgba_frame->data, rgba_frame->linesize, out_buffer_,
                                 AV_PIX_FMT_RGBA,
                                 video_width_, video_height_, 1);

            // 通过设置宽高限制缓冲区中的像素数量，而非屏幕的物理显示尺寸。
            // 如果缓冲区与物理屏幕的显示尺寸不相符，则实际显示可能会是拉伸，或者被压缩的图像
            int result = ANativeWindow_setBuffersGeometry(native_window_, video_width_,
                                                          video_height_, WINDOW_FORMAT_RGBA_8888);
            if (result < 0) {
                LOGE(TAG, "Player Error : Can not set native window buffer");
                avcodec_close(codec_ctx);
                avcodec_free_context(&codec_ctx);
                av_free(out_buffer_);
                break;
            }

            auto frame = av_frame_alloc();
            auto packet = av_packet_alloc();

            struct SwsContext* data_convert_context = sws_getContext(
                    video_width_, video_height_, codec_ctx->pix_fmt,
                    video_width_, video_height_, AV_PIX_FMT_RGBA,
                    SWS_BICUBIC, nullptr, nullptr, nullptr);
            while (!is_stop_) {
                LOGD(TAG, "front seek time_ms_ = %ld, last_decode_time_ms_ = %ld", time_ms_,
                     last_decode_time_ms_);
                if (!is_seeking_ && (time_ms_ > last_decode_time_ms_ + 1000 ||
                                     time_ms_ < last_decode_time_ms_ - 50)) {
                    is_seeking_ = true;
                    LOGD(TAG, "seek frame time_ms_ = %ld， last_decode_time_ms_ = %ld", time_ms_,
                         last_decode_time_ms_);
                    // 传进去的是指定帧带有 time_base 的时间戳，所以是要将原来的 times_ms 按照上面获取时的计算方式反推算出时间戳
                    av_seek_frame(av_format_context, video_stream_index,
                                  time_ms_ * stream->time_base.den / 1000, AVSEEK_FLAG_BACKWARD);
                }
                // 读取视频一帧（完整的一帧），获取的是一帧视频的压缩数据，接下来才能对其进行解码
                ret = av_read_frame(av_format_context, packet);
                if (ret < 0) {
                    avcodec_flush_buffers(codec_ctx);
                    av_seek_frame(av_format_context, video_stream_index,
                                  time_ms_ * stream->time_base.den / 1000, AVSEEK_FLAG_BACKWARD);
                    LOGD(TAG, "ret < 0, condition_.wait(lock)");
                    // 防止解码最后一帧时视频已经没有数据
                    on_decode_frame_(last_decode_time_ms_);
                    condition_.wait(lock);
                }
                if (packet->size) {
                    Decode(codec_ctx, packet, frame, stream, lock, data_convert_context,
                           rgba_frame);
                }
            }
            // 释放资源
            sws_freeContext(data_convert_context);
            av_free(out_buffer_);
            av_frame_free(&rgba_frame);
            av_frame_free(&frame);
            av_packet_free(&packet);

        }
        avcodec_close(codec_ctx);
        avcodec_free_context(&codec_ctx);

    } while (false);
    avformat_close_input(&av_format_context);
    avformat_free_context(av_format_context);
    ANativeWindow_release(native_window_);
    delete this;
}

bool VideoDecoder::DecodeFrame(long time_ms) {
    LOGD(TAG, "DecodeFrame time_ms = %ld", time_ms);
    if (last_decode_time_ms_ == time_ms || time_ms_ == time_ms) {
        LOGD(TAG, "DecodeFrame last_decode_time_ms_ == time_ms");
        return false;
    }
    if (last_decode_time_ms_ >= time_ms && last_decode_time_ms_ <= time_ms + 50) {
        return false;
    }
    time_ms_ = time_ms;
    condition_.notify_all();
    return true;
}

void VideoDecoder::Release() {
    is_stop_ = true;
    condition_.notify_all();
}

/**
 * 获取当前的毫秒级时间
 */
int64_t VideoDecoder::GetCurrentMilliTime(void) {
    struct timeval tv{};
    gettimeofday(&tv, nullptr);
    return tv.tv_sec * 1000.0 + tv.tv_usec / 1000.0;
}

