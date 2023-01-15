//
// Created by 16278 on 2022/5/23.
//

#include "AVCodecAudioLocalthread.h"

AVCodecAudioLocalthread::AVCodecAudioLocalthread() {

}

void AVCodecAudioLocalthread::setFilePath(char* path) {
    this->filePath = path;

    if (initAudioCodec()<0){
        release();
    }
}

int AVCodecAudioLocalthread::initAudioCodec() {
    LOGI("Audio 开始");

    avformat_network_init(); //初始化FFMPEG  调用了这个才能正常适用编码器和解码器

    //Allocate an AVFormatContext.
    pFormatCtx = avformat_alloc_context();


    if (avformat_open_input(&pFormatCtx, this->filePath, NULL, NULL) != 0) {
        LOGI("can't open the file.");
        //emit initAudioFinish();
        return -1;
    }

    if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
        LOGI("Could't find stream infomation.");
        //emit initAudioFinish();
        return -1;
    }

    ///循环查找视频中包含的流信息，直到找到视频类型的流
    ///便将其记录下来 保存到videoStream 和 audioStream变量中
    ///这里我们现在只处理视频流  音频流先不管他
    for (int i = 0; i < pFormatCtx->nb_streams; i++) {
        if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
            audioStream = i;
        }
    }

    ///判断有音频格式是否支持，如果支持在接下来的解码中解析音频数据
    if (audioStream == -1) {
        LOGI("音频 audiostream");
        //emit initAudioFinish();
        return -1;
    }

    if (init2AudioCodec() == -1) {
        return -1;
    }

    int y_size = aCodecCtx->width * aCodecCtx->height;
    packet = (AVPacket *) malloc(sizeof(AVPacket)); //分配一个packet
    av_new_packet(packet, y_size); //分配packet的数据

    av_dump_format(pFormatCtx, 0, this->filePath, 0); //输出视频信息

    goPlay(true);

    return 0;
}

int AVCodecAudioLocalthread::init2AudioCodec() {
    aCodecCtx = pFormatCtx->streams[audioStream]->codec;
    if (!aCodecCtx)
    {
        LOGI("音频 CodecCtx失败");
        //emit initAudioFinish();
        return -1;
    }
    AVCodec *aCodec = avcodec_find_decoder(aCodecCtx->codec_id);
    if (!aCodec)
    {
        LOGI( "音频 Codec失败");
        //emit initAudioFinish();
        return -1;
    }

    if (avcodec_open2(aCodecCtx,aCodec, NULL) < 0){
        LOGI("avcodec_open error (Audio)");
        //emit initAudioFinish();
        return -1;
    }
    a_out_buffer = (uint8_t *) malloc(2 * MAX_AUDIO_FRAME_SIZE);
    aAudioCovertCtx = swr_alloc();
    swr_init(aAudioCovertCtx);
    out_channer_nb = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO);
    aAvFrame = av_frame_alloc();

    /**
     * 回调到java onAudioCreate
     */
    if (avCallback){

        JNIEnv *t_env = nullptr;
        jint state = vm->AttachCurrentThread((JNIEnv**)&t_env, NULL);
        if(t_env == nullptr || state < 0) {
            vm->DetachCurrentThread();
            LOGE("env is nullptr. state is %d\n", state);
            return -1;
        }

        jclass clazz = t_env->GetObjectClass(avCallback);
        jmethodID onPreparedId = t_env->GetMethodID(clazz, "onAudioCreate", "(II)V");
        t_env->CallVoidMethod(avCallback, onPreparedId, MAX_AUDIO_FRAME_SIZE, out_channer_nb);
        t_env->DeleteLocalRef(clazz);
    }

    return 1;
}

void AVCodecAudioLocalthread::goPlay(bool play) {
    this->play = play;

    int out_buffer_size;
    double sleepTime = 0;
    while (this->play)
    {
        if(!this->play){
            break;
        }
        if (av_read_frame(pFormatCtx, packet) < 0)
        {
            break; //这里认为视频读取完了
        }

        if(packet->stream_index == audioStream) {
            int got_frame = 0, ret;
            ret = avcodec_decode_audio4(aCodecCtx, aAvFrame, &got_frame, packet);
            if (ret < 0){
                LOGI( "decode audio error.");
                return;
            }
            if (got_frame > 0){
                LOGI( "decode audio SUCCESS");
                swr_convert(
                        aAudioCovertCtx,
                        &a_out_buffer,
                        MAX_AUDIO_FRAME_SIZE * 2,
                        (const uint8_t **) aAvFrame->data,
                        aAvFrame->nb_samples);
                out_buffer_size = av_samples_get_buffer_size(NULL, out_channer_nb, aAvFrame->nb_samples,
                                                             AV_SAMPLE_FMT_S16, 1);
                sleepTime = (aCodecCtx->sample_rate*16*2/8)/out_buffer_size;
                audioPts = packet->pts;
                av_usleep(40 * 1000);
//                if(audioOutput->bytesFree() < out_buffer_size) {
//                    av_usleep(sleepTime * 1000);
//                    audioDevice->write((const char*)a_out_buffer, out_buffer_size);
//                } else {
//                    audioDevice->write((const char*)a_out_buffer, out_buffer_size);
//                }

                if (aAvFrame->pkt_dts != AV_NOPTS_VALUE) {
                    audioTimeStamp = aAvFrame->pkt_dts;
                } else if (aAvFrame->pts != AV_NOPTS_VALUE) {
                    audioTimeStamp = aAvFrame->pts;
                } else {
                    audioTimeStamp = 0;
                }
                audioTimeStamp = (int)((audioTimeStamp * av_q2d(pFormatCtx->streams[audioStream]->time_base)) * 1000);


                /**
                 * 回调返回到java数据
                 */
                 if (avCallback) {
                     LOGI( "decode audio callback");
                     JNIEnv *t_env = nullptr;
                     jint state = vm->AttachCurrentThread((JNIEnv**)&t_env, NULL);
                     if(t_env == nullptr || state < 0) {
                         vm->DetachCurrentThread();
                         LOGE("env is nullptr. state is %d\n", state);
                         return;
                     }

                     jbyteArray audiodata = t_env->NewByteArray(out_buffer_size);
                     t_env->SetByteArrayRegion(audiodata,0,out_buffer_size,
                                             reinterpret_cast<const jbyte *>(a_out_buffer));
                     jclass clazz = t_env->GetObjectClass(avCallback);
                     jmethodID onAudioAvailableId = t_env->GetMethodID(clazz, "onAudioAvailable", "([B)V");
                     t_env->CallVoidMethod(avCallback, onAudioAvailableId, audiodata);
                     t_env->DeleteLocalRef(clazz);
                     t_env->DeleteLocalRef(audiodata);
                 } else {
                     LOGI( "decode audio callback null");
                 }

            }
        }
        if (packet){
            av_free_packet(packet);
        }
    }

}

void AVCodecAudioLocalthread::release() {
    goPlay(false);
    if (a_out_buffer){
        av_free(a_out_buffer);
    }
    if (aAvFrame) {
        av_free(aAvFrame);
    }
    if (aCodecCtx) {
        avcodec_close(aCodecCtx);
    }
    if (pFormatCtx) {
        avformat_close_input(&pFormatCtx);
    }
    if (vm){
        vm->DestroyJavaVM();
    }
}

void AVCodecAudioLocalthread::setJniEnv(JNIEnv *env) {
    this->env = env;
}

void AVCodecAudioLocalthread::setAvCallback(jobject call_back) {
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
