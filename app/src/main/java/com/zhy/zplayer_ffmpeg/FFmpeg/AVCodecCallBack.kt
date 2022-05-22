package com.zhy.zplayer_ffmpeg.FFmpeg

/**
 *Created by ZQL
 *Created on 2022/5/22
 **/
interface AVCodecCallBack {
    //视频数据准备回调
    fun onPrepared(width: Int, height: Int)

    //音频数据准备回调
    fun onAudioCreate(sampleRateInHz: Int, nb_channals: Int)

    //数据回调
    fun onFrameAvailable(data: ByteArray?)
    fun onAudioAvailable(data: ByteArray?)

    //播放结束回调
    fun onPlayFinished()
}