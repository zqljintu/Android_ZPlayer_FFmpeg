package com.zhy.zplayer_ffmpeg

import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import com.zhy.zplayer_ffmpeg.FFmpeg.AVCodecHandler

class MainActivity : AppCompatActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
        AVCodecHandler.initFFmpeg()
    }
}