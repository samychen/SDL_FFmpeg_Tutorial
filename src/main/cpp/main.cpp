//
// Created by Administrator on 2017/10/17 0017.
//
#include <jni.h>
#include "ffmpeg.h"
#include <android/log.h>
#include <SDL.h>

#define LOG_I(...) __android_log_print(ANDROID_LOG_ERROR , "main", __VA_ARGS__)
SDL_Renderer* sdlRender;
SDL_Texture * sdlTexture;
int width = 640;
int height = 352;
AVFormatContext *pFormatCtx;
extern "C"
JNIEXPORT void JNICALL
Java_com_tz_dream_sdl_ffmpeg_demo_MainActivity_playrtmp(JNIEnv *env, jobject instance) {
    av_register_all();
    avfilter_register_all();
    avformat_network_init();

    icodec=avformat_alloc_context();
    const char *configuration = avcodec_configuration();
    __android_log_print(ANDROID_LOG_INFO,"main","%s",configuration);

}extern "C"
JNIEXPORT jint JNICALL
Java_com_tz_dream_sdl_ffmpeg_demo_MainActivity_InitVideo(JNIEnv *env, jobject instance) {
    int av_stream_index = -1;
    for (int i = 0; i < pFormatCtx->nb_streams; ++i) {
        //循环遍历每一流
        //视频流、音频流、字幕流等等...
        if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO){
            //找到了
            av_stream_index = i;
            break;
        }
    }
    if (av_stream_index == -1){
        __android_log_print(ANDROID_LOG_INFO,"main","%s","没有找到视频流");
        return -1;
    }
    AVCodecContext* avcodec_context = pFormatCtx->streams[av_stream_index]->codec;
    AVCodec* avcodec = avcodec_find_decoder(avcodec_context->codec_id);
    if (avcodec == NULL){
        __android_log_print(ANDROID_LOG_INFO,"main","%s","没有找到视频解码器");
        return -1;
    }
    //第五步：打开解码器
    int avcodec_open2_result = avcodec_open2(avcodec_context,avcodec,NULL);
    if (avcodec_open2_result != 0){
        char* error_info;
        av_strerror(avcodec_open2_result, error_info, 1024);
        __android_log_print(ANDROID_LOG_INFO,"main","异常信息：%s",error_info);
        return -1;
    }
    __android_log_print(ANDROID_LOG_INFO,"main","解码器名称：%s",avcodec->name);
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* sdlWindow = SDL_CreateWindow("Dream开车",
                     SDL_WINDOWPOS_CENTERED,
                     SDL_WINDOWPOS_CENTERED,
                     width ,
                     height,
                     SDL_WINDOW_OPENGL);
    if (sdlWindow == NULL){
        LOG_I("窗口创建失败");
        return 0;
    }
    sdlRender = SDL_CreateRenderer(sdlWindow, -1, SDL_RendererFlags::SDL_RENDERER_ACCELERATED);
//    sdlRender = SDL_CreateRenderer(sdlWindow, -1, 0);
    sdlTexture = SDL_CreateTexture(sdlRender, SDL_PIXELFORMAT_YV12, SDL_TEXTUREACCESS_STREAMING, width, height);
//    sdlTexture = SDL_CreateTexture(sdlRender, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, width, height);
    return 1;
}

int Display(uint8_t ** data, int* linesize){
    void* piexels = nullptr;
    int pitch;

    int ret = SDL_LockTexture(sdlTexture, NULL, &piexels, &pitch);
    if(ret < 0) return ret;
    uint8_t*	yuv[3] = {
            (uint8_t*)piexels,(uint8_t*)piexels + pitch * height,
            (uint8_t*)piexels + pitch * height + ((pitch >> 1) * (height >> 1))
    };

    for (int i = 0; i < height; i++)
    {
        memcpy(yuv[0] + i * pitch, data[0] + i * linesize[0], linesize[0]);
        if (i % 2 == 0)
        {
            memcpy(yuv[1] + (i >> 1) * (pitch >> 1), data[2] + (i >> 1) * linesize[2], linesize[2]);
            memcpy(yuv[2] + (i >> 1) * (pitch >> 1), data[1] + (i >> 1) * linesize[1], linesize[1]);
        }
    }
    SDL_UnlockTexture(sdlTexture);
    SDL_RenderClear(sdlRender);
    SDL_RenderCopy(sdlRender, sdlTexture, NULL, NULL);
    SDL_RenderPresent(sdlRender);
    return 1;
}
void Close(){
    SDL_CloseAudio();

//    if(blockingBuffer)
//    {
//        delete blockingBuffer;
//    }
}
int Read(uint8_t *dst, int size)
{
    if(!dst || size == 0) return 0;
    std::lock_guard<std::mutex> lk(lock);
    int bytesRead = size < bytesCanRead ? size : bytesCanRead;
    if(bytesRead <= bufferSize - readIndex)
    {
        memcpy(dst,buffer + readIndex,bytesRead);
        readIndex += bytesRead;
        if(readIndex == bufferSize) readIndex = 0;
    }
    else
    {
        int bytesHead = bufferSize - readIndex;
        memcpy(dst, buffer + readIndex,bytesHead);
        int bytesTail = bytesRead - bytesHead;
        memcpy(dst + readIndex,buffer, bytesTail);
        readIndex = bytesTail;
    }
    bytesCanRead -= bytesRead;
    return bytesRead;
}
void  fill_audio(void *udata,Uint8 *stream,int len){
    if(udata != NULL)
    {
        CGBlockRingBuffer* blockingBuffer = (CGBlockRingBuffer*)userdata;
        auto len1 = 0;
        while (len > 0)
        {
            len1 = blockingBuffer->Read((uint8_t*)stream + len1,len);
            len = len - len1;
        }

    }
}
extern "C"
JNIEXPORT jint JNICALL
Java_com_tz_dream_sdl_ffmpeg_demo_MainActivity_InitAudio(JNIEnv *env, jobject instance) {
    int audioStream=-1;
    for(int i=0; i < pFormatCtx->nb_streams; i++)
        if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_AUDIO) {
            audioStream=i;
            break;
        }
    if(audioStream==-1) {
        __android_log_print(ANDROID_LOG_INFO, "main", "%s", "没有找到音频流");
        return -1;
    }
    AVCodecContext* pCodecCtx=pFormatCtx->streams[audioStream]->codec;
    AVCodec * pCodec=avcodec_find_decoder(pCodecCtx->codec_id);
    if(pCodec==NULL) {
        __android_log_print(ANDROID_LOG_INFO, "main", "%s", "没有找到音频解码器");
        return -1;
    }
    // Open codec
    int avcodec_open2_result = avcodec_open2(pCodecCtx, pCodec, NULL);
    if(avcodec_open2_result<0) {
        char *error_info;
        av_strerror(avcodec_open2_result, error_info, 1024);
        __android_log_print(ANDROID_LOG_INFO, "main", "异常信息3：%s", error_info);
        return -1;
    }
    SDL_Init(SDL_INIT_AUDIO);
    blockingBuffer = new CGBlockRingBuffer();
    blockingBuffer->Init(4 * 1024);
    SDL_AudioSpec wanted;
    wanted.freq = 44100;
    wanted.format = AUDIO_S16SYS;
    wanted.channels = pCodecCtx->channels;
    wanted.silence = 0;
    wanted.samples = 1024;
    wanted.callback = callback;
    wanted.userdata = pCodecCtx;
    //	devid_out = SDL_OpenAudioDevice(NULL, SDL_FALSE, &wanted, &spec, SDL_AUDIO_ALLOW_ANY_CHANGE);
    auto r = (SDL_OpenAudio(&wanted,NULL));
    SDL_PauseAudio(0);
    return 1;
}extern "C"
JNIEXPORT void JNICALL
Java_com_tz_dream_sdl_ffmpeg_demo_MainActivity_Init(JNIEnv *env, jobject instance) {
    const char *configuration = avcodec_configuration();
    __android_log_print(ANDROID_LOG_INFO,"main","%s",configuration);
    av_register_all();
    avfilter_register_all();
    avformat_network_init();
    pFormatCtx = avformat_alloc_context();
    int avformat_open_result = avformat_open_input(&pFormatCtx, cinputFilePath, NULL, NULL);
    if (avformat_open_result != 0) {
        char *error_info;
        av_strerror(avformat_open_result, error_info, 1024);
        __android_log_print(ANDROID_LOG_INFO, "main", "异常信息1：%s", error_info);
        return ;
    }
    int avformat_find_stream_info_result = avformat_find_stream_info(pFormatCtx, NULL);
    if (avformat_find_stream_info_result < 0) {
        char *error_info;
        av_strerror(avformat_find_stream_info_result, error_info, 1024);
        __android_log_print(ANDROID_LOG_INFO, "main", "异常信息2：%s", error_info);
        return ;
    }

}