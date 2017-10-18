//
// Created by Administrator on 2017/10/17 0017.
//
#ifndef __FFMPEG_H__
#define __FFMPEG_H__

#include "info.h"

extern "C"
{
#include "libavformat/avformat.h"
#include "libavformat/avio.h"
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
#include "libavutil/avutil.h"
#include "libavutil/mathematics.h"
#include "libswresample/swresample.h"
#include "libavutil/opt.h"
#include "libavutil/channel_layout.h"
#include "libavutil/samplefmt.h"
#include "libavdevice/avdevice.h"  //摄像头所用
#include "libavfilter/avfilter.h"
#include "libavutil/error.h"
#include "libavutil/mathematics.h"
#include "libavutil/time.h"
#include "inttypes.h"
#include "stdint.h"
};

#pragma comment(lib,"avformat.lib")
#pragma comment(lib,"avcodec.lib")
#pragma comment(lib,"avdevice.lib")
#pragma comment(lib,"avfilter.lib")
#pragma comment(lib,"avutil.lib")
#pragma comment(lib,"postproc.lib")
#pragma comment(lib,"swresample.lib")
#pragma comment(lib,"swscale.lib")


//#define INPUTURL   "test.flv"
#define INPUTURL     "rtmp://192.168.1.107/oflaDemo/test"
//#define OUTPUTURL  "testnew.flv";
#define OUTPUTURL    "rtmp://192.168.1.107/oflaDemo/testnew"

//video param
extern int m_dwWidth;
extern int m_dwHeight;
extern double m_dbFrameRate;  //帧率
extern AVCodecID video_codecID;
extern AVPixelFormat video_pixelfromat;
extern char spspps[100];
extern int spspps_size;

//audio param
extern int m_dwChannelCount; //声道
extern int m_dwBitsPerSample; //样本
extern int m_dwFrequency;     //采样率
extern AVCodecID audio_codecID;
extern int audio_frame_size;

#define AUDIO_ID            0                                                 //packet 中的ID ，如果先加入音频 pocket 则音频是 0  视频是1，否则相反
#define VEDIO_ID            1

extern int nRet;                                                              //状态标志
extern AVFormatContext* icodec;
extern AVInputFormat* ifmt;
extern char szError[256];                                                     //错误字符串
extern AVFormatContext* oc ;                                                  //输出流context
extern AVOutputFormat* ofmt;
extern AVStream* video_st;
extern AVStream* audio_st;
extern AVCodec *audio_codec;
extern AVCodec *video_codec;
extern int video_stream_idx;
extern int audio_stream_idx;
extern AVPacket pkt;
extern AVBitStreamFilterContext * vbsf_aac_adtstoasc;                         //aac->adts to asc过滤器
static int  m_nVideoTimeStamp = 0;
static int  m_nAudioTimeStamp = 0;

int InitInput(char * Filename,AVFormatContext ** iframe_c,AVInputFormat** iinputframe);
int InitOutput();
AVStream * Add_output_stream_2(AVFormatContext* output_format_context,AVMediaType codec_type_t, AVCodecID codecID,AVCodec **codec);
int OpenCodec(AVStream * istream, AVCodec * icodec); //打开编解码器
void read_frame(AVFormatContext *oc);    //从文件读取一帧数据根据ID读取不同的帧
void write_frame_3(AVFormatContext *oc,int ID,AVPacket pkt_t); //这个是根据传过来的buf 和size 写入文件
int Es2Mux_2(); //通过时间戳控制写入音视频帧顺序
int UintInput();
int UintOutput();

#endif
