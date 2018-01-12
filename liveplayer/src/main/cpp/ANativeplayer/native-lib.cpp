#include <jni.h>
#include <string>


#include "Log.h"

#include "Video2.h"


//http://dranger.com/ffmpeg/tutorial05.html


extern "C" {


Video *video = 0;
Audio *audio = 0;

ANativeWindow *window = 0;

char *path = 0;
pthread_t p_tid;
int isPlay = 0;


void call_video_play(AVFrame *frame) {
    if (!window) {
        return;
    }
    ANativeWindow_Buffer window_buffer;
    if (ANativeWindow_lock(window, &window_buffer, 0)) {
        return;
    }
    uint8_t *dst = (uint8_t *) window_buffer.bits;
    int dstStride = window_buffer.stride * 4;
    uint8_t *src = frame->data[0];
    int srcStride = frame->linesize[0];
    for (int i = 0; i < window_buffer.height; ++i) {
        memcpy(dst + i * dstStride, src + i * srcStride, srcStride);
    }
    ANativeWindow_unlockAndPost(window);

}


void *process(void *args) {
    av_register_all();
    avformat_network_init();
    AVFormatContext *ifmt_ctx = 0;
    LOGI("set url:%s", path);
    avformat_open_input(&ifmt_ctx, path, 0, 0);
    avformat_find_stream_info(ifmt_ctx, 0);
    //查找流
    for (int i = 0; i < ifmt_ctx->nb_streams; i++) {

        AVCodecParameters *parameters = ifmt_ctx->streams[i]->codecpar;
        AVCodec *dec = avcodec_find_decoder(parameters->codec_id);
        AVCodecContext *codec = avcodec_alloc_context3(dec);
        avcodec_parameters_to_context(codec, parameters);
        avcodec_open2(codec, dec, 0);


        if (parameters->codec_type == AVMEDIA_TYPE_AUDIO) {
            audio->setCodec(codec);
            audio->index = i;
            audio->time_base = ifmt_ctx->streams[i]->time_base;
        } else if (parameters->codec_type == AVMEDIA_TYPE_VIDEO) {
            video->setCodec(codec);
            video->index = i;
            video->time_base = ifmt_ctx->streams[i]->time_base;
            if (window)
                ANativeWindow_setBuffersGeometry(window, video->codec->width,
                                                 video->codec->height,
                                                 WINDOW_FORMAT_RGBA_8888);
        }
    }
    video->play(audio);
    audio->play();
    isPlay = 1;
    AVPacket *packet = av_packet_alloc();
    int ret = 0;
    while (isPlay) {
        ret = av_read_frame(ifmt_ctx, packet);
        if (ret == 0) {
            if (video && packet->stream_index == video->index) {
                video->enQueue(packet);
            } else if (audio && packet->stream_index == audio->index) {
                audio->enQueue(packet);
            }
            av_packet_unref(packet);
        } else if (ret == AVERROR_EOF) {
            //读取完毕 但是不一定播放完毕
            while (isPlay) {
                if (video->queue.empty() && audio->queue.empty()) {
                    break;
                }
//                LOGI("等待播放完成");
                av_usleep(10000);
            }
            break;
        } else {
            break;
        }
    }
    isPlay = 0;
    if (video && video->isPlay)
        video->stop();
    if (audio && audio->isPlay)
        audio->stop();
    avformat_free_context(ifmt_ctx);
    av_packet_free(&packet);
    LOGI("PROCESS EXIT");
    pthread_exit(0);
}

JNIEXPORT void JNICALL
Java_com_samychen_gracefulwrapper_liveplayer_DNPlayer_native_1play(
        JNIEnv *env,
        jobject instance, jstring url_) {
    const char *url = env->GetStringUTFChars(url_, 0);


    path = (char *) malloc(strlen(url) + 1);
    memset(path, 0, strlen(url) + 1);
    memcpy(path, url, strlen(url));

    video = new Video;
    audio = new Audio;
    video->setPlayCall(call_video_play);
    pthread_create(&p_tid, 0, process, 0);
    env->ReleaseStringUTFChars(url_, url);
}


JNIEXPORT void JNICALL
Java_com_samychen_gracefulwrapper_liveplayer_DNPlayer_native_1set_1display(JNIEnv *env, jobject instance,
                                                        jobject surface) {
    if (window) {
        ANativeWindow_release(window);
        window = 0;
    }
    window = ANativeWindow_fromSurface(env, surface);
    if (video && video->codec)
        ANativeWindow_setBuffersGeometry(window, video->codec->width,
                                         video->codec->height,
                                         WINDOW_FORMAT_RGBA_8888);
}

JNIEXPORT void JNICALL
Java_com_samychen_gracefulwrapper_liveplayer_DNPlayer_native_1stop(JNIEnv *env, jobject instance) {
    if (path) {
        free(path);
        path = 0;
    }
    if (isPlay) {
        isPlay = 0;
        pthread_join(p_tid, 0);
    }
    if (video) {
        if (video->isPlay) {
            video->stop();
        }
        delete (video);
        video = 0;
    }
    if (audio) {
        if (audio->isPlay) {
            audio->stop();
        }
        delete (audio);
        audio = 0;
    }

}

JNIEXPORT void JNICALL
Java_com_samychen_gracefulwrapper_liveplayer_DNPlayer_native_1release(JNIEnv *env, jobject instance) {
    if (window)
        ANativeWindow_release(window);
    window = 0;
}

}