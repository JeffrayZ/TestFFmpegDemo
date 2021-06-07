#include <jni.h>
#include <string>
#include <android/native_window_jni.h>
#include <zconf.h>
#include <android/log.h>

#define LOGI(FORMAT, ...) __android_log_print(ANDROID_LOG_INFO,"zhanfeifei",FORMAT,##__VA_ARGS__);
#define LOGE(FORMAT, ...) __android_log_print(ANDROID_LOG_ERROR,"zhanfeifei",FORMAT,##__VA_ARGS__);

// 采样率
#define MAX_AUDIO_FRME_SIZE 48000 * 4

typedef uint8_t *string;
extern "C" {
    // 解码
#include <libavcodec/avcodec.h>
// 缩放
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavutil/channel_layout.h>
    // 封装格式
#include <libavformat/avformat.h>
// 重采样
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
Java_com_test_testffmpegdemo_ZFFmpegPlayer_native_1sound(JNIEnv *env, jobject thiz,
                                                         jstring input_path, jstring out_put) {
    const char *inputPath = env->GetStringUTFChars(input_path, nullptr);
    const char *outputPath = env->GetStringUTFChars(out_put, nullptr);

    // 初始化ffmpeg网络请求
    // 为什么要初始化？为了播放在线
    avformat_network_init();
    // 解码
    // AVFormatContext，主要是获取视频流、音频流或者其他的字母流
    // 总上下文
    AVFormatContext *avFormatContext = avformat_alloc_context();
    // 打开音频文件，返回值代表是否成功，0成功  非0失败
    int ret = avformat_open_input(&avFormatContext, inputPath, nullptr, nullptr);
    if(ret != 0){
        LOGI("%s","无法打开音频文件")
        return;
    }
    // 获取输入文件信息
    if(avformat_find_stream_info(avFormatContext, nullptr) < 0){
        LOGI("%s","无法获取输入文件信息")
        return;
    }
    //视频时长（单位：微秒us，转换为秒需要除以1000000）
    int audio_stream_idx=-1;
    for (int i = 0; i < avFormatContext->nb_streams; ++i) {
        if (avFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audio_stream_idx=i;
            break;
        }
    }
    AVCodecParameters *codecpar = avFormatContext->streams[audio_stream_idx]->codecpar;
    //找到解码器
    AVCodec *dec = avcodec_find_decoder(codecpar->codec_id);
    //创建上下文
    AVCodecContext *codecContext = avcodec_alloc_context3(dec);
    avcodec_parameters_to_context(codecContext, codecpar);
    avcodec_open2(codecContext, dec, nullptr);
    SwrContext *swrContext = swr_alloc();
    // 输入样本格式，也是采样位数
    AVSampleFormat in_sample = codecContext->sample_fmt;
    // 输入采样率
    int in_sample_rate = codecContext->sample_rate;
    // 输入声道布局，也就是通道数
    uint64_t in_ch_layout = codecContext->channel_layout;
    // 输出采样格式
    AVSampleFormat out_sample = AV_SAMPLE_FMT_S16;
    // 输出采样率
    int out_sample_rate = 44100;
    // 输出声道布局 AV_CH_LAYOUT_STEREO 双通道
    uint64_t out_ch_layout = AV_CH_LAYOUT_STEREO;
    // 给转换上下文设置输入输出参数
    swr_alloc_set_opts(swrContext,out_ch_layout,out_sample,out_sample_rate
            ,in_ch_layout,in_sample,in_sample_rate,0,nullptr);
    // 设置结束后初始化转换器其他的默认参数
    swr_init(swrContext);
    // 输出缓冲区
    // av_malloc 声明内存 通道数乘以采样率
    uint8_t *out_buffer = (uint8_t *)(av_malloc(2 * 44100));
    FILE *fp_pcm = fopen(outputPath, "wb");
    //读取包  压缩数据
    AVPacket *packet = av_packet_alloc();
    int count = 0;
    while (av_read_frame(avFormatContext, packet)>=0) {
        // 从音频流中取得 packet 压缩
        avcodec_send_packet(codecContext, packet);
        // 解压缩数据  frame 未压缩
        AVFrame *frame = av_frame_alloc();
        // 将 packet 转换成 frame
        int ret = avcodec_receive_frame(codecContext, frame);
        // frame
        if (ret == AVERROR(EAGAIN))
            continue;
        else if (ret < 0) {
            LOGE("解码完成");
            break;
        }
        if (packet->stream_index!= audio_stream_idx) {
            continue;
        }
        LOGE("正在解码%d",count++);
        // 将音频文件转换成统一格式
        swr_convert(swrContext,&out_buffer,2*44100,
                    (const uint8_t **)frame->data,frame->nb_samples);
        int out_channerl_nb= av_get_channel_layout_nb_channels(out_ch_layout);
        // 获取每一帧的大小 align 0对其1不对齐   这里不对齐，只剔除无用数据
        int out_buffer_size =  av_samples_get_buffer_size(nullptr, out_channerl_nb, frame->nb_samples, out_sample, 1);
        // 1 是字节流的最小单位，代表字节
        fwrite(out_buffer,1,out_buffer_size,fp_pcm);
    }
    fclose(fp_pcm);
    av_free(out_buffer);
    swr_free(&swrContext);
    avcodec_close(codecContext);
    avformat_close_input(&avFormatContext);
    env->ReleaseStringUTFChars(input_path, inputPath);
    env->ReleaseStringUTFChars(out_put, outputPath);
}