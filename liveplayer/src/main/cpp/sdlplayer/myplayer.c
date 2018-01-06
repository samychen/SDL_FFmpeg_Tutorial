//
// Created by Administrator on 2017/11/5 0005.
//
#include <stdio.h>
#include "SDL.h"
#include <pthread.h>

#include <android/log.h>
#include "libavcodec/avcodec.h"

#define  LOGE(format,...)  __android_log_print(ANDROID_LOG_ERROR , "main", __VA_ARGS__)

#define BITS_PER_PIXEL 32
#define SDL_AUDIO_BUFFER_SIZE 1024*2

int VideoWidth=0;
int VideoHeight=0;
SDL_Surface *Screen;
pthread_t Task_Video =0;
pthread_t Task_Audio =0;
int Isrun_Video =1;
#define RUNNING 1
#define STOP 0
#define EXIT -1

int init_stream(const char* path) {
    int res = 1, i;
    int numBytes;
    char pathStr[100]={0};
    ff_play_register_all();
    strcpy(pathStr,path);
    res = ff_play_open_and_initctx(pathStr);
    if(res < 0){
        LOGE("_open_and_initctx res[%d]",res);
        return res;
    }
    res = ff_play_begin_read_thread();
    VideoWidth =ff_play_getvideo_width();
    VideoHeight=ff_play_getvideo_height();
    LOGE("begin thread res[%d], videowidth[%d], height[%d]",res,VideoWidth,VideoHeight);
    return res;
}

void* my_play_video_thread(void *arg) {
    int i,res=-1;
    int ret;
    double pkt_pts;
    while(Isrun_Video){
        SDL_LockSurface(Screen);
        VideoWidth =ff_play_getvideo_width();
        VideoHeight=ff_play_getvideo_height();
        ret = ff_play_picture_fill(Screen->pixels,VideoWidth, VideoHeight,PIX_FMT_BGRA);
        if(ret <0){
            return NULL;
        }
        ff_play_getvideopkt2display(NULL);
        SDL_UnlockSurface(Screen);
        SDL_UpdateRect(Screen, 0, 0, 0, 0);
    }
    Isrun_Video = EXIT;
    return NULL;
}

void  my_play_sdl_audio_callback2(void *userdata, Uint8 *stream, int max_len) {

    AVCodecContext *aCodecCtx = (AVCodecContext *) userdata;
    int len, len1, audio_size;
    static uint8_t audio_buf[(AVCODEC_MAX_AUDIO_FRAME_SIZE * 3) / 2];
    static unsigned int audio_buf_size = 0;
    static unsigned int audio_buf_index = 0;
    len = max_len;
    while (len > 0) {
        if (audio_buf_index >= audio_buf_size) {
            audio_size = ff_play_getaudiopkt2play((void *) audio_buf, max_len);
            if (audio_size <= 0) {
                audio_buf_size = 1024; // arbitrary?
                memset(audio_buf, 0, audio_buf_size);
            } else {
                audio_buf_size = audio_size;
            }
            audio_buf_index = 0;
        }
        len1 = audio_buf_size - audio_buf_index;
        if (len1 > len)
            len1 = len;
//注意，用这个混音函数，不要直接用memcpy。否则声音会失真
        SDL_MixAudio(stream,audio_buf,len1,SDL_MIX_MAXVOLUME);
//memcpy(stream, (uint8_t *) audio_buf + audio_buf_index, len1);
        len -= len1;
        stream += len1;
        audio_buf_index += len1;
    }
}

int main(int argc, void* argv[]){
    int i=0,res=0,x,y;
    char str_path[100]={0};
    SDL_Rect rect;
    SDL_AudioSpec   wanted_spec, spec;
    SDL_Event      event;

    strcpy(str_path,argv[1]);
    res = init_stream(str_path);
    if(res <0 ){
        fprintf(stderr, "open file fail code[%d]", res);
        return 0;
    }
    LOGE("open file ok!!");

    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
        fprintf(stderr, "Could not initialize SDL - %s", SDL_GetError());
        exit(1);
    }
    LOGE("init video ok!!");
    Screen = SDL_SetVideoMode(VideoWidth,VideoHeight, BITS_PER_PIXEL, SDL_HWSURFACE);
    if(!Screen) {
        fprintf(stderr, "SDL: could not set video mode - exiting");
        exit(1);
    }
    LOGE("Screen ok BitsPerPixel[%d], BytesPerPixel[%d], pitch[%d] x[%d]y[%d]w[%d]h[%d]!!! ",
         Screen->format->BitsPerPixel, Screen->format->BytesPerPixel, Screen->pitch,
         Screen->clip_rect.x, Screen->clip_rect.y, Screen->clip_rect.w, Screen->clip_rect.h);
    LOGE("video is ready !!\n");

    wanted_spec.freq = ff_play_getaudio_samplerate();
    wanted_spec.format = AUDIO_S16LSB;
    wanted_spec.channels = ff_play_getaudio_channels();
    wanted_spec.silence = 0;
    wanted_spec.samples = SDL_AUDIO_BUFFER_SIZE;
    wanted_spec.callback = my_play_sdl_audio_callback2;
    wanted_spec.userdata = NULL;
    LOGE("samplerate[%d],channle[%d]", wanted_spec.freq,wanted_spec.channels);
    if (SDL_OpenAudio(&wanted_spec, &spec) < 0) {
        fprintf(stderr, "SDL_OpenAudio: %s\n", SDL_GetError());
        return -1;
    }
    LOGE("ready to play!!");

    SDL_PauseAudio(0);
    res = pthread_create(&Task_Video, NULL, my_play_video_thread, NULL);
    while(1){
        SDL_PollEvent(&event);
        switch(event.type) {
            case SDL_QUIT:
                Isrun_Video = STOP;
                while(Isrun_Video != EXIT);
                SDL_CloseAudio();
                ff_play_FreeAll();
                SDL_Quit();
                return 0;
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                    case SDLK_RIGHT:
                        ff_play_jump(1);
// LOGE("SDLK_RIGHT DOWN!!!!!");
                        break;
                    case SDLK_LEFT:
                        ff_play_jump(-1);
// LOGE("SDLK_LEFT DOWN!!!!!");
                        break;
                    case SDLK_UP:
// LOGE("SDLK_UP DOWN!!!!!");
                        break;
                    case SDLK_DOWN:
// LOGE("SDLK_DOWN DOWN!!!!!");
                        break;
                }break;
            default:
                break;
        }
    }
    return 0;
}
