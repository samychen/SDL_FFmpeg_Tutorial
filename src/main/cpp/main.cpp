//
// Created by Administrator on 2017/10/17 0017.
//
#include <jni.h>
#include "ffmpeg.h"
#include <android/log.h>
#define LOG_I(...) __android_log_print(ANDROID_LOG_ERROR , "main", __VA_ARGS__)

extern "C"
JNIEXPORT void JNICALL
Java_com_tz_dream_sdl_ffmpeg_demo_MainActivity_playrtmp(JNIEnv *env, jobject instance) {
    av_register_all();
    avformat_network_init();
    InitInput(INPUTURL,&icodec,&ifmt);
    InitOutput();
    LOG_I("--------程序运行开始----------\n");
    Es2Mux_2();
    UintOutput();
    UintInput();
    LOG_I("--------程序运行结束----------\n");
}