package com.zhy.zplayer_ffmpeg

import android.media.AudioFormat
import android.media.AudioManager
import android.media.AudioTrack
import android.os.Bundle
import android.util.Log
import androidx.appcompat.app.AppCompatActivity
import com.zhy.zplayer_ffmpeg.FFmpeg.AVCodecCallBack
import com.zhy.zplayer_ffmpeg.FFmpeg.AVCodecHandler

class MainActivity : AppCompatActivity(),AVCodecCallBack{

    private var mAudioTrack: AudioTrack? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
        val fileDir = "/sdcard/in2.mp4"
        val threradVideo = Thread(){
            kotlin.run {
                AVCodecHandler.setVideoCodecFilePath(fileDir, this)
            }
        }
        val threradAudio = Thread(){
            kotlin.run {
                AVCodecHandler.setAudioCodecFilePath(fileDir, this)
            }
        }
//        threradVideo.start()
        threradAudio.start()
    }

    override fun onPrepared(width: Int, height: Int) {

    }

    override fun onAudioCreate(sampleRateInHz: Int, nb_channals: Int) {
        Log.d("MainActivity", "onAudioCreate")
        val channaleConfig: Int = if (nb_channals == 1) {
            AudioFormat.CHANNEL_OUT_MONO
        } else if (nb_channals == 2) {
            AudioFormat.CHANNEL_OUT_STEREO
        } else {
            AudioFormat.CHANNEL_OUT_MONO
        } //通道数
        val buffersize = AudioTrack.getMinBufferSize(
            sampleRateInHz,
            channaleConfig, AudioFormat.ENCODING_PCM_16BIT
        )
        mAudioTrack = AudioTrack(
            AudioManager.STREAM_MUSIC, sampleRateInHz, channaleConfig,
            AudioFormat.ENCODING_PCM_16BIT, buffersize, AudioTrack.MODE_STREAM
        )
        mAudioTrack?.play()
    }

    override fun onFrameAvailable(data: ByteArray?) {

    }

    override fun onAudioAvailable(data: ByteArray?) {
        Log.d("MainActivity", "onAudioAvailable")
        if (mAudioTrack != null && mAudioTrack?.playState == AudioTrack.PLAYSTATE_PLAYING) {
            data?.let { mAudioTrack?.write(it, 0, data.size) }
        }
    }

    override fun onPlayFinished() {

    }


}