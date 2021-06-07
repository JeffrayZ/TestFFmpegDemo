#include <jni.h>
#include <string>
#include <android/native_window_jni.h>
#include <zconf.h>
#include <android/log.h>

#define LOGI(FORMAT, ...) __android_log_print(ANDROID_LOG_INFO,"zhanfeifei",FORMAT,##__VA_ARGS__);
#define LOGE(FORMAT, ...) __android_log_print(ANDROID_LOG_ERROR,"zhanfeifei",FORMAT,##__VA_ARGS__);

#define MAX_AUDIO_FRME_SIZE 48000 * 4

typedef uint8_t *string;
extern "C" {
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavutil/channel_layout.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_test_testffmpegdemo_MainActivity_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    return env->NewStringUTF(av_version_info());
}

extern "C"
JNIEXPORT void JNICALL
Java_com_test_testffmpegdemo_ZFFmpegPlayer_native_1start(JNIEnv *env, jobject thiz,
                                                         jstring intput_path,
                                                         jobject surface) {
    const char *inputPath = env->GetStringUTFChars(intput_path, NULL);
    // 初始化ffmpeg网络请求
    // 为什么要初始化？为了播放直播
    avformat_network_init();
    // 解码
    // AVFormatContext，主要是获取视频流、音频流或者其他的字母流
    // 视频流不能直接播放
    // 所以要解压 AVCodecContext,主要是获取视频的信息，比如宽高，视频编码等等
    // AVCodec 视频解码器 将视频解压成原始的YUV数据
    // YUV数据最终转换渲染到surfaceView
    // 转换需要转换上下文 SwsContext
    // 所以代码如下：
    AVFormatContext *avFormatContext = avformat_alloc_context();
    AVDictionary *avDictionary = nullptr;
    // 设置3秒超时
    av_dict_set(&avDictionary, "timeout", "3000000", 0);
    // 强制指定AVFormatContext中AVInputFormat的。这个参数一般情况下可以设置为NULL，这样FFmpeg可以自动检测AVInputFormat。
    // 输入文件的封装格式
    // av_find_input_format("avi")
    // 返回值代表是否成功，0成功  非0失败
    int ret = avformat_open_input(&avFormatContext, inputPath, nullptr, &avDictionary);
    // if (ret) {
    //  return;
    // }
    // 通知FFmpeg解析流
    avformat_find_stream_info(avFormatContext, nullptr);
    // 索引，视频时长（单位：微秒us，转换为秒需要除以1000000）
    int video_stream_idx = -1;
    for (int i = 0; i < avFormatContext->nb_streams; ++i) {
        // 拿到遍历的第几个流，获取流的参数，再获取流的类型
        // 判断类型是不是video类型，也就是判断是不是视频流类型
        if (avFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            // 记录索引
            video_stream_idx = i;
            break;
        }
    }

    // 获取流的参数
    AVCodecParameters *avCodecParameters = avFormatContext->streams[video_stream_idx]->codecpar;
    // 拿到流的解码器解码器
    AVCodec *avCodec = avcodec_find_decoder(avCodecParameters->codec_id);
    // 获取解码器上下文
    AVCodecContext *avCodecContext = avcodec_alloc_context3(avCodec);
    // 将解码器参数copy到解码器上下文
    avcodec_parameters_to_context(avCodecContext, avCodecParameters);
    // 打开解码器
    avcodec_open2(avCodecContext, avCodec, nullptr);
    // 获取包，实例化 AVPacket,这是压缩后的数据，里面包含视频和音频
    AVPacket *avPacket = av_packet_alloc();
    // 定义一个转换上下文，有转换上下文才能将YUV数据转化成rgb数据，并绘制到surfaceview上
    /**
     * 重视速度：SWS_FAST_BILINEAR   SWS_POINT
     * 重视质量：SWS_BICUBIC   SWS_SPLINE   SWS_LANCZOS
     * 缩小
     * 重视速度：SWS_FAST_BILINEAR   SWS_POINT
     * 重视质量：SWS_GAUSS   SWS_BILINEAR
     * 重视锐度：SWS_BICUBIC   SWS_SPLINE  SWS_LANCZOS
     * */
    SwsContext *swsContext = sws_getContext(avCodecContext->width, avCodecContext->height,
                                            avCodecContext->pix_fmt,
                                            avCodecContext->width, avCodecContext->height,
                                            AV_PIX_FMT_RGBA,
                                            SWS_BILINEAR, nullptr, nullptr, nullptr);
    // 底层渲染对象,通过这个绘制到 surfaceview 上
    ANativeWindow *aNativeWindow = ANativeWindow_fromSurface(env, surface);
    // 缓冲区
    ANativeWindow_Buffer outBuffer;
    // 创建新的窗口用于视频显示
    // ANativeWindow
    int frameCount = 0;
    ANativeWindow_setBuffersGeometry(aNativeWindow, avCodecContext->width, avCodecContext->height,
                                     WINDOW_FORMAT_RGBA_8888);
    // 循环读取，从流中读取数据包，并存入 AVPacket
    while (av_read_frame(avFormatContext, avPacket) >= 0) {
        // 从 avPacket 中读取YUV画面
        avcodec_send_packet(avCodecContext, avPacket);
        // 未压缩的YUV数据
        AVFrame *avFrame = av_frame_alloc();
        ret = avcodec_receive_frame(avCodecContext, avFrame);
        if (ret == AVERROR(EAGAIN)) {
            continue;
        } else if (ret < 0) {
            LOGE("解码完成");
            break;
        }
        // 接收的容器
        uint8_t *dst_data[0];
        // 每一行的首地址
        int dst_linesize[0];
        av_image_alloc(dst_data, dst_linesize, avCodecContext->width, avCodecContext->height,
                       AV_PIX_FMT_RGBA, 1);
        if (avPacket->stream_index == video_stream_idx) {
            LOGE("解码视频");
            // 非零   正在解码
            if (ret == 0) {
                // 绘制之前   配置一些信息  比如宽高   格式
                // 绘制
                // ========== 锁住
                ANativeWindow_lock(aNativeWindow, &outBuffer, nullptr);
                // h264   ----yuv          RGBA
                // 转为指定的YUV420P
                // 绘制
                sws_scale(swsContext, reinterpret_cast<const uint8_t *const *>(avFrame->data),
                          avFrame->linesize, 0, avFrame->height, dst_data,
                          dst_linesize);
                // rgb_frame是有画面数据
                // 渲染
                auto *dst = (uint8_t *) outBuffer.bits;
                // 输入源（rgb）的
                uint8_t *src_data = dst_data[0];
                // 拿到一行有多少字节 rgba
                int dst_stride = outBuffer.stride * 4;
                int src_linesize = dst_linesize[0];
                auto firstWindown = static_cast<uint8_t *>(outBuffer.bits);
                for (int i = 0; i < outBuffer.height; ++i) {
                    // 内存拷贝 来进行渲染  一行一行拷贝
                    memcpy(firstWindown + i * dst_stride, src_data + i * src_linesize, dst_stride);
                }
                // ========== 解锁
                ANativeWindow_unlockAndPost(aNativeWindow);
                usleep(1000 * 16);
                av_frame_free(&avFrame);
            }
        }
        LOGE("正在解码%d", frameCount++);
    }
    ANativeWindow_release(aNativeWindow);
    avcodec_close(avCodecContext);
    avformat_free_context(avFormatContext);
    env->ReleaseStringUTFChars(intput_path, inputPath);
}