package com.test.testffmpegdemo

import android.view.Surface
import android.view.SurfaceHolder
import android.view.SurfaceView

class ZFFmpegPlayer() : SurfaceHolder.Callback2 {
    companion object {
        init {
            System.loadLibrary("zffmpeg")
        }
    }

    // 绘制需要什么  路径、surfaceView

    private var surfaceHolder: SurfaceHolder?=null

    fun setSurfaceView(surfaceView:SurfaceView){
        if(surfaceHolder != null){
            this.surfaceHolder?.removeCallback(this)
        }
        this.surfaceHolder = surfaceView.holder
        this.surfaceHolder?.addCallback(this)
    }

    override fun surfaceCreated(holder: SurfaceHolder) {

    }

    override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) {
        this.surfaceHolder = holder
    }

    override fun surfaceDestroyed(holder: SurfaceHolder) {
    }

    override fun surfaceRedrawNeeded(holder: SurfaceHolder) {
    }

    fun start(inputPath: String) {
        native_start(inputPath,surfaceHolder?.surface)
    }

    external fun native_start(inputPath: String, surface: Surface?)
}