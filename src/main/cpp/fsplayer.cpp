//
// Created by Administrator on 2017/11/2 0002.
//

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"

}
#include <android/log.h>
#include "SDL.h"
#include "SDL_thread.h"

#include "PacketQueue.h"
#include "Audio.h"
#include "Media.h"
#include "VideoDisplay.h"
#define LOG_I(...) __android_log_print(ANDROID_LOG_ERROR , "main", __VA_ARGS__)

bool quit = false;

int main(int argv, char* argc[])
{
    av_register_all();

    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER);

    char* filename = "E:\\Wildlife.wmv";
    //char* filename = "F:\\test.rmvb";
    MediaState media(filename);

    if (media.openInput())
        SDL_CreateThread(decode_thread, "", &media); // 创建解码线程，读取packet到队列中缓存

    media.audio->audio_play(); // create audio thread

    media.video->video_play(&media); // create video thread

    AVStream *audio_stream = media.pFormatCtx->streams[media.audio->stream_index];
    AVStream *video_stream = media.pFormatCtx->streams[media.video->stream_index];

    double audio_duration = audio_stream->duration * av_q2d(audio_stream->time_base);
    double video_duration = video_stream->duration * av_q2d(video_stream->time_base);
    LOG_I("audio时长：%lf",audio_duration);
    LOG_I("video时长：%lf",video_duration);

    SDL_Event event;
    while (true) // SDL event loop
    {
        SDL_WaitEvent(&event);
        switch (event.type)
        {
            case FF_QUIT_EVENT:
            case SDL_QUIT:
                quit = 1;
                SDL_Quit();

                return 0;
                break;

            case FF_REFRESH_EVENT:
                video_refresh_timer(&media);
                break;

            default:
                break;
        }
    }

    getchar();
    return 0;
}