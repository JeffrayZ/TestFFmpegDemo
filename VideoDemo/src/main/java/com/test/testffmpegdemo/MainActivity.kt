package com.test.testffmpegdemo

import android.content.Context
import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.view.SurfaceView
import android.view.View
import android.view.WindowManager
import android.widget.TextView
import android.widget.Toast
import java.io.File
import java.io.FileOutputStream

class MainActivity : AppCompatActivity() {
    private lateinit var surfaceView: SurfaceView
    private lateinit var zfFmpegPlayer: ZFFmpegPlayer

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
        loadMp4(this,"input.mp4")
        window.setFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON,WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
        surfaceView = findViewById(R.id.surfaceView)
        zfFmpegPlayer = ZFFmpegPlayer()
        zfFmpegPlayer.setSurfaceView(surfaceView = surfaceView)
    }

    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
    external fun stringFromJNI(): String



    fun open(view: View) {
        val file = File(cacheDir,"input.mp4")
        zfFmpegPlayer.start(file.absolutePath)
    }

    fun loadMp4(context: Context, fileName: String): String? {
        try {
            val cacheDir = context.cacheDir
            if (!cacheDir.exists()) {
                cacheDir.mkdir()
            }
            val outFile = File(cacheDir, fileName)
            if (!outFile.exists()) {
                val res = outFile.createNewFile()
                if (res) {
                    val `is` = context.assets.open(fileName)
                    val os = FileOutputStream(outFile)
                    val buffer = ByteArray(`is`.available())
                    var byteCount: Int
                    while (`is`.read(buffer).also { byteCount = it } != -1) {
                        os.write(buffer, 0, byteCount)
                    }
                    os.flush()
                    `is`.close()
                    os.close()
                    Toast.makeText(context, "下载成功", Toast.LENGTH_SHORT).show()
                    return outFile.absolutePath
                }
            } else {
                Toast.makeText(context, "下载成功", Toast.LENGTH_SHORT).show()
                return outFile.absolutePath
            }
        } catch (e: Exception) {
            e.printStackTrace()
        }
        return null
    }
}