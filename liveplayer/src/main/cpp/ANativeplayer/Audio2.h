//
// Created by liuxiang on 2017/9/24.
//

#ifndef DNPLAYER_AUDIO_H
#define DNPLAYER_AUDIO_H

#include <SLES/OpenSLES_Android.h>
#include <queue>

extern "C" {

#include "libavformat/avformat.h"
#include "libswresample/swresample.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
#include "libavutil/time.h"


#define MAX_AUDIO_FRME_SIZE 48000 * 4
#define A_CH_LAYOUT AV_CH_LAYOUT_STEREO
#define A_PCM_BITS AV_SAMPLE_FMT_S16
#define A_SAMPLE_RATE 44100


class Audio {
public:
    Audio();

    ~Audio();

    int enQueue(const AVPacket *packet);

    int deQueue(AVPacket *packet);

    void play();

    void stop();

    void setCodec(AVCodecContext *codec);


    double getClock();

    int decodeAudio();

    int createPlayer();



public:
    AVRational time_base;
    int index;
    AVCodecContext *codec;
    double clock;
    int isPlay;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    std::queue<AVPacket *> queue;

    pthread_t p_playid;

    SLObjectItf engineObject;
    SLEngineItf engineEngine;
    SLEnvironmentalReverbItf outputMixEnvironmentalReverb;
    SLObjectItf outputMixObject;
    SLObjectItf bqPlayerObject;
    SLEffectSendItf bqPlayerEffectSend;
    SLVolumeItf bqPlayerVolume;
    SLPlayItf bqPlayerPlay;
    SLAndroidSimpleBufferQueueItf bqPlayerBufferQueue;

    SwrContext *swr_ctx;

    uint8_t *buff;
    int channels;
};
}
#endif //DNPLAYER_AUDIO_H
