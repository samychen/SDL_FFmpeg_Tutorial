//
// Created by Administrator on 2017/11/5 0005.
//
#ifndef __FF_PLAY_H__
#define __FF_PLAY_H__

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavutil/error.h"

#define EFF_PLAY_BASE 1000
#define EFF_PLAY_FIND_CODEC    EFF_PLAY_BASE + 1
#define EFF_PLAY_OPEN_CODEC    EFF_PLAY_BASE + 2

void ff_play_register_all(void);

int ff_play_FreeAll(void);

void ff_play_SaveFrame(AVFrame *pFrame, char*  path, int width, int height, int iFrame);

void ff_play_SaveAudio(AVFrame *pFrame, char*  path, int bufsize);

int ff_play_open_and_initctx(const char *pathStr);

int ff_play_getaudiopkt2play(void *audio_buf, int max_len);

void ff_play_getvideopkt2display(void *arg);

void* ff_play_readpkt_thread(void *arg);

int ff_play_begin_read_thread(void);

int  ff_play_picture_fill(void* pixels,int width,int height,enum PixelFormat pix_fmt);

int ff_play_getvideo_height(void);
int ff_play_getvideo_width(void);
int ff_play_getaudio_samplerate(void);
int ff_play_getaudio_channels(void);

void ff_play_jump(int second);

#endif
