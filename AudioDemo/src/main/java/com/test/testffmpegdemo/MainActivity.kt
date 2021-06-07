package com.test.testffmpegdemo

import android.content.Context
import android.os.Bundle
import android.view.View
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import java.io.File
import java.io.FileOutputStream

class MainActivity : AppCompatActivity() {
    private lateinit var zfFmpegPlayer: ZFFmpegPlayer

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
        loadMp3(this, "input.mp3")
        zfFmpegPlayer = ZFFmpegPlayer()
    }

    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
    external fun stringFromJNI(): String


    fun open(view: View) {
        val file1 = File(cacheDir, "input.mp3")
        val file2 = File(cacheDir, "output.pcm")
        zfFmpegPlayer.sound(file1.absolutePath, file2.absolutePath)
    }

    fun loadMp3(context: Context, fileName: String): String? {
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