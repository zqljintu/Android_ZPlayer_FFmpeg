//
// Created by 16278 on 2022/5/23.
//

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

                ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture,packet);
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
                    if (sleepTime < 40){
                        av_usleep(sleepTime * 1000);
                    } else {
                        av_usleep(40 * 1000);
                    }
                    LOGI("video decode success %d", index++);
                }
            }
        }
    }

}

int AVCodecVideoLocalthread::initVideoCodec() {
    LOGI("video 开始");

    av_register_all(); //初始化FFMPEG  调用了这个才能正常使用编码器和解码器

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

    int dstW = pCodecCtx->width;
    int dstH = pCodecCtx->height;

    img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height,
                                     pCodecCtx->pix_fmt, dstW, dstH,
                                     AV_PIX_FMT_RGB32, SWS_BICUBIC, NULL, NULL, NULL);

    numBytes = avpicture_get_size(AV_PIX_FMT_RGB32, pCodecCtx->width,pCodecCtx->height);

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
