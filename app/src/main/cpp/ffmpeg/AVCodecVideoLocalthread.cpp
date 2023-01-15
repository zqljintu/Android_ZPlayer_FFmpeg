//
// Created by 16278 on 2022/5/23.
//

#include <mutex>
#include "AVCodecVideoLocalthread.h"

AVCodecVideoLocalthread::AVCodecVideoLocalthread() {

}

void AVCodecVideoLocalthread::setFile(char* filePath) {
    this->filePath = filePath;
    if (initVideoCodec() < 0){
        release();
    }
}

void AVCodecVideoLocalthread::goPlay(bool play) {
    this->play = play;
    int index =0;

    while (this->play)
    {
        if(!this->play){
            break;
        }
        if (av_read_frame(pFormatCtx, packet) >= 0){

            if (packet->stream_index == videoStream) {

                ret = avcodec_decode_video(pCodecCtx, pFrame, &got_picture,packet);
                if (ret < 0) {
                    LOGI("decode video error.");
                    return;
                }
                if (got_picture) {
                    sws_scale(img_convert_ctx,
                              (uint8_t const * const *) pFrame->data,
                              pFrame->linesize, 0, pCodecCtx->height, pFrameRGB->data,
                              pFrameRGB->linesize);
                    if (pFrame->pkt_dts != AV_NOPTS_VALUE) {
                        videoTimeStamp = pFrame->pkt_dts;
                    } else if (pFrame->pts != AV_NOPTS_VALUE) {
                        videoTimeStamp = pFrame->pts;
                    } else {
                        videoTimeStamp = 0;
                    }
                    float currentTime = videoTimeStamp * av_q2d(pFormatCtx->streams[videoStream]->time_base);
                    int percent = (int) (currentTime / fileTimes * 100);
                    videoTimeStamp =  (int)((videoTimeStamp * av_q2d(pFormatCtx->streams[videoStream]->time_base)) * 1000);
                    LOGI("video decode success %d", index++);
                    if (avCallback) {

                        JNIEnv *t_env = nullptr;
                        jint state = vm->AttachCurrentThread((JNIEnv**)&t_env, NULL);
                        if(t_env == nullptr || state < 0) {
                            vm->DetachCurrentThread();
                            LOGE("env is nullptr. state is %d\n", state);
                            return;
                        }

                        //获取数据大小 宽高等数据
                        int dataSize = pCodecCtx->height * (pFrameRGB->linesize[0] + pFrameRGB->linesize[1]);
                        jbyteArray data = t_env->NewByteArray(dataSize);
                        t_env->SetByteArrayRegion(data, 0, dataSize,
                                                reinterpret_cast<const jbyte *>(out_buffer));
                        // onFrameAvailable 回调
                        jclass clazz = t_env->GetObjectClass(avCallback);
                        jmethodID onFrameAvailableId = env->GetMethodID(clazz, "onFrameAvailable", "([B)V");
                        t_env->CallVoidMethod(avCallback, onFrameAvailableId, data);
                        t_env->DeleteLocalRef(clazz);
                        t_env->DeleteLocalRef(data);
                    } else {
                        updateVideoFrame(pFrameRGB);
                    }
                    if (sleepTime < 40){
                        av_usleep(sleepTime * 1000);
                    } else {
                        av_usleep(40 * 1000);
                    }
                }
            }
        }
    }

}

int AVCodecVideoLocalthread::initVideoCodec() {
    LOGI("video 开始");

    avformat_network_init(); //初始化FFMPEG  调用了这个才能正常使用编码器和解码器

    //Allocate an AVFormatContext.
    pFormatCtx = avformat_alloc_context();

    if (avformat_open_input(&pFormatCtx, this->filePath, NULL, NULL) != 0) {
        LOGI("can't open the file.");
        return -1;
    }

    fileTimes = pFormatCtx->duration/1000000.0;//转化为秒

    if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
        LOGI("Could't find stream infomation.");
        return -1;
    }

    videoStream = -1;
    ///循环查找视频中包含的流信息，直到找到视频类型的流
    ///便将其记录下来 保存到videoStream 和 audioStream变量中
    ///这里我们现在只处理视频流  音频流先不管他
    for (i = 0; i < pFormatCtx->nb_streams; i++) {
        if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStream = i;
            break;
        }
    }

    ///如果videoStream为-1 说明没有找到视频流
    if (videoStream == -1) {
        LOGI("Didn't find a video stream.");
        return -1;
    }


    ///查找解码器
    pCodecCtx = pFormatCtx->streams[videoStream]->codec;
    pCodec = avcodec_find_decoder(pCodecCtx->codec_id);


    if (pCodec == NULL) {
        LOGI("Codec not found.");
        return -1;
    }

    ///打开解码器
    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
        LOGI("Could not open codec.");
        return -1;
    }

    pFrame = av_frame_alloc();
    pFrameRGB = av_frame_alloc();

    if (dstW == 0) {
        dstW = pCodecCtx->width;
    }
    if (dstH == 0) {
        dstH = pCodecCtx->height;
    }

    img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height,
                                     pCodecCtx->pix_fmt, dstW, dstH,
                                     AV_PIX_FMT_RGBA, SWS_BICUBIC, NULL, NULL, NULL);

    numBytes = avpicture_get_size(AV_PIX_FMT_RGBA, pCodecCtx->width,pCodecCtx->height);

    out_buffer = (uint8_t *) av_malloc(numBytes * sizeof(uint8_t));
    avpicture_fill((AVPicture *) pFrameRGB, out_buffer, AV_PIX_FMT_RGB32,
                   pCodecCtx->width, pCodecCtx->height);

    int y_size = pCodecCtx->width * pCodecCtx->height;

    packet = (AVPacket *) malloc(sizeof(AVPacket)); //分配一个packet
    av_new_packet(packet, y_size); //分配packet的数据

    av_dump_format(pFormatCtx, 0, this->filePath, 0); //输出视频信息

    goPlay(true);

    return 0;
}

void AVCodecVideoLocalthread::release() {
    goPlay(false);
    if (out_buffer) {
        av_free(out_buffer);
    }
    if (pFrameRGB) {
        av_free(pFrameRGB);
    }
    if (pFrame) {
        av_free(pFrame);
    }
    if (pCodec) {
        av_free(pCodec);
    }
    if (pCodecCtx) {
        avcodec_close(pCodecCtx);
    }
    if (pFormatCtx) {
        avformat_close_input(&pFormatCtx);
    }
}

void AVCodecVideoLocalthread::setJniEnv(JNIEnv *env) {
    this->env = env;
}

void AVCodecVideoLocalthread::setAvCallback(jobject call_back) {
    env -> GetJavaVM(&vm);
    if (vm -> AttachCurrentThread(&env, NULL) != JNI_OK){
        return;
    }

    if (avCallback != NULL) {
        env->DeleteGlobalRef(avCallback);
        avCallback = NULL;
    }
    avCallback = env->NewGlobalRef(call_back);
}

void AVCodecVideoLocalthread::setANativeWindow(ANativeWindow *nwindow) {
//    std::lock_guard<std::mutex> lock(windowLock);
    this->native_window = nwindow;
    if (this->native_window != 0) {
        ANativeWindow_release(nwindow);
    }
    this->windowWidth = ANativeWindow_getWidth(this->native_window);
    this->windowHeight = ANativeWindow_getHeight(this->native_window);
    ANativeWindow_setBuffersGeometry(native_window, dstW, dstH, WINDOW_FORMAT_RGBA_8888);
}

void AVCodecVideoLocalthread::updateVideoFrame(AVFrame *frameRgba) {
    if (this->native_window == 0) {
        return;
    }
    LOGE("H1>>%d",windowWidth);
    LOGE("H2>>%d",dstW);
    ANativeWindow_lock(native_window, &window_buffer, 0);
    // 获取stride
    uint8_t *dst = (uint8_t *) window_buffer.bits;
    int dstStride = window_buffer.stride * 4;
    uint8_t *src = (frameRgba->data[0]);
    int srcStride = frameRgba->linesize[0];

    // 由于window的stride和帧的stride不同,因此需要逐行复制
    for (int i = 0; i < dstH; i++) {
        // 原画大小播放
        // 逐行拷贝内存数据，但要进行偏移，否则视频会拉伸变形
        // (i + (windowHeight - videoHeight) / 2) * dstStride 纵向偏移，确保视频纵向居中播放
        // (dstStride - srcStride) / 2 横向偏移，确保视频横向居中播放
        memcpy(dst + (i + (windowHeight - dstH) / 2) * dstStride +
               (dstStride - srcStride) / 2, src + i * srcStride,
               srcStride);
    }
    ANativeWindow_unlockAndPost(native_window);
}

void AVCodecVideoLocalthread::setDstSize(int width, int height) {
    this->dstW = width;
    this->dstH = height;
}
