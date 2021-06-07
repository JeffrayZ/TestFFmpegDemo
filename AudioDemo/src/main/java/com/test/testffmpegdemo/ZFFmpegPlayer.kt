package com.test.testffmpegdemo

class ZFFmpegPlayer {
    companion object {
        init {
            System.loadLibrary("zffmpeg")
        }
    }

    fun sound(inputPath: String,outPut:String) {
        native_sound(inputPath,outPut)
    }

    external fun native_sound(inputPath: String, outPut: String)
}