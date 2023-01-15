//
// Created by DELL on 2020/12/3.
//

#include "FFmpegPullStream.h"

int FFmpegPullStream::init(JNIEnv *env, jstring url) {
    if (frameCallback == NULL){
        LOGE("init Callback == null");
        return -1;
    }

    //URL地址获取
    const char* temp = env->GetStringUTFChars(url,NULL);
    char input_str[500] = {0};
    strcpy(input_str,temp);
    env->ReleaseStringUTFChars(url,temp);

    //Register
    avformat_network_init();
    //NetWork
    avformat_network_init();
    //注册库中所有可用的文件格式和编码器
    avcodec_register_all();
    avdevice_register_all();


    pAvFrame = av_frame_alloc();
    aAvFrame = av_frame_alloc();
    pFrameNv21 = av_frame_alloc();
    pFormatCtx = avformat_alloc_context();

    if (avformat_open_input(&pFormatCtx, in_filename, NULL, NULL) < 0){
        LOGE("Could't open input");
        return -1;
    }

    if (avformat_find_stream_info(pFormatCtx, NULL) < 0){
        LOGE("Could't find stream");
        return -2;
    }

    //遍历各个流，找到第一个视频流,并记录该流的编码信息
    for (unsigned int i = 0; i < pFormatCtx->nb_streams; i++)
    {
        if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            //这里获取到的videoindex的结果为1.
            videoIndex = i;
            break;
        }

    }
    //遍历各个流，找到第一个音频流,并记录该流的编码信息
    for (unsigned int j = 0; j < pFormatCtx->nb_streams; j++)
    {
        if (pFormatCtx->streams[j]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
            //这里获取到的videoindex的结果为1.
            audioIndex = j;
            break;
        }
    }
    if (audioIndex == -1) {
        LOGE("AudioIndex == -1");
    }
    initAudio(env);
    if (videoIndex == -1) {
        LOGE("VideoIndex == -1");
        return -3;
    }
    initVideo(env);
    return 0;
}

/**
 * 使用Surface播放视频数据
 * @return
 */
int FFmpegPullStream::onDecodeAndRendering() {
   /* bool stop = false;
    int count = 0;
    while (!stop) {
        if (av_read_frame(pFormatCtx, pPacket) >= 0) {
            //解码
            int gotPicCount = 0;
            int decode_video2_size = avcodec_decode_video2(pCodecCtx, pAvFrame, &gotPicCount,
                                                           pPacket);
            LOGI("decode_video2_size = %d , gotPicCount = %d", decode_video2_size, gotPicCount);
            LOGI("pAvFrame->linesize  %d  %d %d", pAvFrame->linesize[0], pAvFrame->linesize[1],
                 pCodecCtx->height);

            //绑定输出buffer
            int width = pCodecCtx->width;
            int height = pCodecCtx->height;
            int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGBA, width, height, 1);
            v_out_buffer = (uint8_t *)av_malloc(numBytes*sizeof(uint8_t));
            av_image_fill_arrays(pRGBFrame->data, pRGBFrame->linesize, v_out_buffer, AV_PIX_FMT_RGBA, width, height, 1);


            if (gotPicCount != 0) {
                count++;
                sws_scale(
                        pImgConvertCtx,
                        (const uint8_t *const *) pAvFrame->data,
                        pAvFrame->linesize,
                        0,
                        pCodecCtx->height,
                        pRGBFrame->data,
                        pRGBFrame->linesize);
                //获取数据大小 宽高等数据
                int dataSize = pCodecCtx->height * (pAvFrame->linesize[0] + pAvFrame->linesize[1]);
                LOGE("pAvFrame->linesize  %d  %d %d %d", pAvFrame->linesize[0],
                     pAvFrame->linesize[1], pCodecCtx->height, dataSize);

            if (ANativeWindow_lock(native_window, &out_buffer, NULL) < 0) {
                LOGE("cannot lock window");
            } else {
                //将图像绘制到界面上，注意这里pFrameRGBA一行的像素和windowBuffer一行的像素长度可能不一致
                //需要转换好，否则可能花屏
                    uint8_t *dst = (uint8_t *) out_buffer.bits;
                    for (int h = 0; h < height; h++)
                    {
                        memcpy(dst + h * out_buffer.stride * 4,
                               v_out_buffer + h * pRGBFrame->linesize[0],
                               pRGBFrame->linesize[0]);
                    }
                    ANativeWindow_unlockAndPost(native_window);

                }
            }


        }
        av_packet_unref(pPacket);
    }*/
    return 0;

}




void FFmpegPullStream::initVideo(JNIEnv *env) {
    pCodecCtx = pFormatCtx->streams[videoIndex]->codec;
    AVCodec *pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
    avcodec_open2(pCodecCtx, pCodec, NULL);

    int width = pCodecCtx->width;
    int height = pCodecCtx->height;
    LOGI("width = %d , height = %d", width, height);
    int numBytes = av_image_get_buffer_size(AV_PIX_FMT_NV21, width, height, 1);
    v_out_buffer = (uint8_t *) av_malloc(numBytes * sizeof(uint8_t));
    av_image_fill_arrays(pFrameNv21->data, pFrameNv21->linesize, v_out_buffer, AV_PIX_FMT_NV21,
                         width,
                         height, 1);
    pImgConvertCtx = sws_getContext(
            pCodecCtx->width,             //原始宽度
            pCodecCtx->height,            //原始高度
            pCodecCtx->pix_fmt,           //原始格式
            pCodecCtx->width,             //目标宽度
            pCodecCtx->height,            //目标高度
            AV_PIX_FMT_NV21,     //目标格式
            SWS_FAST_BILINEAR,            //选择哪种方式来进行尺寸的改变,关于这个参数,可以参考:http://www.cnblogs.com/mmix2009/p/3585524.html
            NULL,
            NULL,
            NULL);
    pPacket = (AVPacket *) av_malloc(sizeof(AVPacket));
    LOGE("Video init success!!");
    //onPrepared 回调
    jclass clazz = env->GetObjectClass(frameCallback);
    jmethodID onPreparedId = env->GetMethodID(clazz, "onPrepared", "(II)V");
    env->CallVoidMethod(frameCallback, onPreparedId, width, height);
    env->DeleteLocalRef(clazz);
}

void FFmpegPullStream::initAudio(JNIEnv *env) {
    aCodecCtx = pFormatCtx->streams[audioIndex]->codec;
    if (!aCodecCtx){
        LOGE("aCodecCtx error");
        return;
    }
    AVCodec  *aCodec = avcodec_find_decoder(aCodecCtx->codec_id);
    if (!aCodec){
        LOGE("aCodex error");
        return;
    }
    if (avcodec_open2(aCodecCtx,aCodec, NULL) < 0){
        LOGE("avcodec_open error (Audio)");
        return;
    }
    a_out_buffer = (uint8_t *) malloc(2 * MAX_AUDIO_FRAME_SIZE);
    //设置格式转换
    aAudioCovertCtx = swr_alloc();
    av_opt_set_int(aAudioCovertCtx, "in_channel_layout",  aCodecCtx->channel_layout, 0);
    av_opt_set_int(aAudioCovertCtx, "out_channel_layout", aCodecCtx->channel_layout,  0);
    av_opt_set_int(aAudioCovertCtx, "in_sample_rate",     aCodecCtx->sample_rate, 0);
    av_opt_set_int(aAudioCovertCtx, "out_sample_rate",    aCodecCtx->sample_rate, 0);
    av_opt_set_sample_fmt(aAudioCovertCtx, "in_sample_fmt",  aCodecCtx->sample_fmt, 0);
    av_opt_set_sample_fmt(aAudioCovertCtx, "out_sample_fmt", AV_SAMPLE_FMT_S16,  0);
    swr_init(aAudioCovertCtx);
    out_channer_nb = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO);
    LOGE("Audio init success!!");
    if (frameCallback == NULL){
        LOGE("frameCallback == null (initAudio)");
        return;
    }
    //onPrepared 回调
    jclass clazz = env->GetObjectClass(frameCallback);
    jmethodID onPreparedId = env->GetMethodID(clazz, "onAudioCreate", "(II)V");
    env->CallVoidMethod(frameCallback, onPreparedId, MAX_AUDIO_FRAME_SIZE, out_channer_nb);
    env->DeleteLocalRef(clazz);
}


void FFmpegPullStream::decodeVideo(JNIEnv *env) {
    //视频解码
    int gotPicCount = 0;
    int decode_video2_size = avcodec_decode_video2(pCodecCtx, pAvFrame, &gotPicCount,
                                                   pPacket);
    LOGI("decode_video2_size = %d , gotPicCount = %d", decode_video2_size, gotPicCount);
    LOGI("pAvFrame->linesize  %d  %d %d", pAvFrame->linesize[0], pAvFrame->linesize[1],
         pCodecCtx->height);
    if (gotPicCount != 0) {
        count++;
        sws_scale(
                pImgConvertCtx,
                (const uint8_t *const *) pAvFrame->data,
                pAvFrame->linesize,
                0,
                pCodecCtx->height,
                pFrameNv21->data,
                pFrameNv21->linesize);
        //获取数据大小 宽高等数据
        int dataSize = pCodecCtx->height * (pAvFrame->linesize[0] + pAvFrame->linesize[1]);
        LOGI("pAvFrame->linesize  %d  %d %d %d", pAvFrame->linesize[0],
             pAvFrame->linesize[1], pCodecCtx->height, dataSize);
        jbyteArray data = env->NewByteArray(dataSize);
        env->SetByteArrayRegion(data, 0, dataSize,
                                reinterpret_cast<const jbyte *>(v_out_buffer));
        // onFrameAvailable 回调
        jclass clazz = env->GetObjectClass(frameCallback);
        jmethodID onFrameAvailableId = env->GetMethodID(clazz, "onFrameAvailable", "([B)V");
        env->CallVoidMethod(frameCallback, onFrameAvailableId, data);
        env->DeleteLocalRef(clazz);
        env->DeleteLocalRef(data);
    }
}

void FFmpegPullStream::decodeAudio(JNIEnv *env) {
    int got_frame = 0, index = 0, ret;
    ret = avcodec_decode_audio4(aCodecCtx,aAvFrame,&got_frame,pPacket);
    if (ret < 0){
        return;
    }
    if (got_frame > 0){
        swr_convert(
                aAudioCovertCtx,
                &a_out_buffer,
                MAX_AUDIO_FRAME_SIZE * 2,
                (const uint8_t **) aAvFrame->data,
                    aAvFrame->nb_samples);
        int out_buffer_size = av_samples_get_buffer_size(NULL, out_channer_nb, aAvFrame->nb_samples,
                                                         AV_SAMPLE_FMT_S16, 1);
        LOGE("out_buffer_size%d",out_buffer_size);
        if (frameCallback == NULL){
            LOGE("frameCallback == null //decodeAudio");
            return;
        }
        jbyteArray audiodata = env->NewByteArray(out_buffer_size);
        env->SetByteArrayRegion(audiodata,0,out_buffer_size,
                                reinterpret_cast<const jbyte *>(a_out_buffer));
        // onFrameAvailable 回调
        jclass clazz = env->GetObjectClass(frameCallback);
        jmethodID onAudioAvailableId = env->GetMethodID(clazz, "onAudioAvailable", "([B)V");
        env->CallIntMethod(frameCallback,onAudioAvailableId,audiodata);
        env->DeleteLocalRef(clazz);
        env->DeleteLocalRef(audiodata);
    }
}

void FFmpegPullStream::start(JNIEnv *env) {
    //开始播放
    isstop = false;
    if (frameCallback == NULL){
        LOGE("framecallback == null");
        return;
    }
    // 读取数据包
    while (!isstop) {
        if (isstop){
            break;
        }
        if (av_read_frame(pFormatCtx, pPacket) >= 0) {
            if (pPacket->stream_index == videoIndex){
                decodeVideo(env);
            } else if (pPacket->stream_index == audioIndex){
                decodeAudio(env);
            } else {

            }
        }
        av_packet_unref(pPacket);
    }
    if (isstop){
        LOGE("isstop >>> OnDestroy");
        onDestroy();
    }
}

void FFmpegPullStream::stop(JNIEnv *env) {
//停止播放
    isstop = true;
    if (frameCallback == NULL) {
        LOGE("stop framecallback == null");
        return;
    }
    jclass clazz = env->GetObjectClass(frameCallback);
    jmethodID onPlayFinishedId = env->GetMethodID(clazz, "onPlayFinished", "()V");
    //发送onPlayFinished 回调
    env->CallVoidMethod(frameCallback, onPlayFinishedId);
    env->DeleteLocalRef(clazz);
}

void FFmpegPullStream::onFrame(JNIEnv *env, jobject play_callback) {
    if (frameCallback != NULL) {
        env->DeleteGlobalRef(frameCallback);
        frameCallback = NULL;
    }
    frameCallback = (env)->NewGlobalRef(play_callback);
}

void FFmpegPullStream::onDestroy() {
    count = 0;
    if (NULL != pImgConvertCtx){
        sws_freeContext(pImgConvertCtx);
    }
    if (NULL != aAudioCovertCtx){
        swr_free(&aAudioCovertCtx);
    }
    if (NULL != pPacket){
        av_free(pPacket);
    }
    if (NULL != pAvFrame){
        av_free(pAvFrame);
    }
    if (NULL != pFrameNv21){
        av_free(pFrameNv21);
    }
    if (NULL != aAvFrame){
        av_free(aAvFrame);
    }
    if (NULL != pCodecCtx){
        avcodec_close(pCodecCtx);
    }
    if (NULL != pFormatCtx){
        avformat_close_input(&pFormatCtx);
    }

}




