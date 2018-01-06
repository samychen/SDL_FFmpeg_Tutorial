//
// Created by Administrator on 2017/11/5 0005.
//
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>
#include <pthread.h>
#include "ff_queue.h"
#include "ff_play.h"

#include <android/log.h>
#define  LOGE(format,...)  __android_log_print(ANDROID_LOG_ERROR , "main", __VA_ARGS__)

AVFormatContext *FormatCtx = NULL;
AVCodecContext *VideoCodecCtx=NULL;
AVCodecContext *AudioCodecCtx=NULL;
AVCodec *Codec=NULL;
AVFrame *FrameVideo=NULL;
AVFrame *FrameVideoRGB=NULL;
AVFrame *FrameAudio=NULL;
enum PixelFormat FrameRGB_Pix_Fmt;
int StreamVideo = -1;
int StreamAudio = -1;
FILE* File_Audio_Record=NULL;
PacketQueue Queue_Audio ={0};
PacketQueue Queue_Video ={0};
pthread_t Task_Read =0;
int Isrun_Read = 0;
double Clock_Audio;
double Clock_Video;
unsigned char  Delay_Muti=0;
#define DELAY_BASE 50
#define INTERVAL_MAX 0.5
#define INTERVAL_ALERT 0.1
int Seek_Time=0;

int Flush_Video=0;
int Flush_Audio=0;

//#define  ONLY_I_FRAME

//保存一帧图像
void ff_play_SaveFrame(AVFrame *FrameVideo, char*  path, int width, int height, int iFrame) {
    FILE *pFile;
    char szFilename[100];
    int  y;
// Open file
    sprintf(szFilename, "%sframed.ppm", path, iFrame);
    pFile=fopen(szFilename, "wb");
    if(pFile==NULL)
        return;
// Write header
    fprintf(pFile, "P6\n%d %d\n255\n", width, height);
// Write pixel data
    for(y=0; y<height; y++)
        fwrite(FrameVideo->data[0]+y*FrameVideo->linesize[0], 1, width*3, pFile);
// Close file
    fclose(pFile);
}
//保存音频到文件
void ff_play_SaveAudio(AVFrame *FrameVideo, char*  path, int bufsize) {
    FILE *pFile;
    char szFilename[100];
    int  y;
// Open file
    if(File_Audio_Record == NULL){
        sprintf(szFilename, "%saudio.pcm", path);
        File_Audio_Record=fopen(szFilename, "wb");
        if(File_Audio_Record==NULL)
            return;
    }
// Write data
    fwrite(FrameVideo->data[0],1,bufsize,File_Audio_Record);//pcm记录
    fflush(File_Audio_Record);
}
//初始化音视频队列
void ff_play_init_queue(void){
    ff_queue_init(&Queue_Audio);
    ff_queue_init(&Queue_Video);
}
//分配帧
void ff_play_allocFrame(){
    FrameVideo = avcodec_alloc_frame();
    FrameVideoRGB = avcodec_alloc_frame();
    FrameAudio = avcodec_alloc_frame();
}
//释放所有资源，首先会停掉读线程
int ff_play_FreeAll(void){
    Isrun_Read = 0;
    while(Isrun_Read >= 0);
    if(FrameVideo != NULL)
        av_free(FrameVideo);
    if(FrameVideoRGB != NULL)
        av_free(FrameVideoRGB);
    if(FrameAudio != NULL)
        av_free(FrameAudio);
    avcodec_close(VideoCodecCtx);
    avcodec_close(AudioCodecCtx);
    if(FormatCtx!=NULL)
        avformat_close_input(&FormatCtx);
    if(File_Audio_Record != NULL)
        fclose(File_Audio_Record);
    return 0;
}

void ff_play_register_all(){
    av_register_all();
    avcodec_register_all();
    avformat_network_init();
}

//打开并初始化上下文以及相关变量 初始化队列，分配帧
int ff_play_open_and_initctx(const char *pathStr){
    int res = 1, i;
    int numBytes;
    res = avformat_open_input(&FormatCtx, pathStr, NULL, NULL);
    if (res < 0) {
        return res;
    }
    res = avformat_find_stream_info(FormatCtx, NULL);
    if (res < 0) {
        return res;
    }
    for (i=0; i < FormatCtx->nb_streams; i++) {
        if (FormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            StreamVideo = i;
        }
        if (FormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
            StreamAudio = i;
        }
        if(StreamVideo >=0 && StreamAudio >=0)
            break;
    }
    if(StreamVideo >=0 ){
        VideoCodecCtx = FormatCtx->streams[StreamVideo]->codec;
        Codec = avcodec_find_decoder(VideoCodecCtx->codec_id);
        if (Codec == NULL) {
            return AVERROR(EFF_PLAY_FIND_CODEC);
        }
        if (Codec->capabilities & CODEC_CAP_TRUNCATED) {
            VideoCodecCtx->flags |= CODEC_FLAG_TRUNCATED;
        }
        if(avcodec_open2(VideoCodecCtx, Codec, NULL) <0){
            return AVERROR(EFF_PLAY_OPEN_CODEC);
        }
    }
    if(StreamAudio >=0 ){
        AudioCodecCtx = FormatCtx->streams[StreamAudio]->codec;
        Codec = avcodec_find_decoder(AudioCodecCtx->codec_id);
        if (Codec == NULL) {
            return AVERROR(EFF_PLAY_FIND_CODEC);
        }
        if (Codec->capabilities & CODEC_CAP_TRUNCATED) {
            AudioCodecCtx->flags |= CODEC_FLAG_TRUNCATED;
        }
        if(avcodec_open2(AudioCodecCtx, Codec, NULL) <0){
            return AVERROR(EFF_PLAY_OPEN_CODEC);
        }
    }
    ff_play_init_queue();
    ff_play_allocFrame();
    return 0;
}
//将pixels 赋给 FrameVideoRGB->data[0]
int  ff_play_picture_fill(void* pixels,int width,int height,enum PixelFormat pix_fmt){
    int res;
    if(FrameVideoRGB->data[0] != pixels){
        res = avpicture_fill((AVPicture *) FrameVideoRGB, pixels, pix_fmt,
                             width, height) <0;
        FrameRGB_Pix_Fmt = pix_fmt;
        return res;
    }
    return 0;
}

int ff_play_getvideo_height(void){
    return VideoCodecCtx != NULL? VideoCodecCtx->height:100;
}
int ff_play_getvideo_width(void){
    return VideoCodecCtx != NULL? VideoCodecCtx->width:100 ;
}
int ff_play_getaudio_samplerate(void){
    return AudioCodecCtx != NULL ? AudioCodecCtx->sample_rate:44100;
}

int ff_play_getaudio_channels(void){
    return AudioCodecCtx != NULL ? AudioCodecCtx->channels:2;
}
//从音频队列取一个包写入到audio_buf
int ff_play_getaudiopkt2play(void *audio_buf, int max_len){
    int ret;
    int len_get = 0,len_left=0;
    int data_size=0;
    uint8_t *pbuf;
    int pkt_size;
    int handle_finished=0,frameFinished = 0;
    AVPacket packet;
    uint8_t *FR_pkt_data_org;

    av_init_packet(&packet);
    pbuf = (uint8_t*)audio_buf;

    do{
        if(Flush_Audio == 1){
            avcodec_flush_buffers(AudioCodecCtx);
            Flush_Audio = 0;

            LOGE("FLUSH OK!!","sss");

        }
        LOGE("11111!!!","sss");

        ret = ff_queue_packet_get(&Queue_Audio,&packet,0);
        if(ret<=0){
            break;
        }
        LOGE("22222 stream_idx[%d], stream_audio[%d]!!!",packet.stream_index ,StreamAudio );
        if (packet.stream_index != StreamAudio){
            av_free_packet(&packet);
            continue;
        }
        LOGE("33333!!!");

        FR_pkt_data_org = packet.data;
        len_left = (max_len > 0 ? max_len : 1000000 );
        while(packet.size>0 && len_left>0) {
            len_get = avcodec_decode_audio4(AudioCodecCtx,FrameAudio,&frameFinished,&packet);
            if (len_get<0){
                break;
            }
            if(frameFinished){
                data_size = av_samples_get_buffer_size(FrameAudio->linesize, AudioCodecCtx->channels,
                                                       FrameAudio->nb_samples,AudioCodecCtx->sample_fmt, 0);
                data_size = ((data_size > len_left && len_left >0 )?len_left:data_size);
                memmove((void *)pbuf,(void *)FrameAudio->data[0],data_size);
            }
            packet.data += len_get;
            packet.size -= len_get;
            pbuf += data_size;
            len_left -= data_size;
            LOGE("55555 decode len_get[%d],len_left[%d] data_size[%d] frameFinished[%d]!!!", \
len_get, len_left,data_size,frameFinished);
        }
        if(packet.pts != AV_NOPTS_VALUE){
            Clock_Audio = av_q2d(FormatCtx->streams[StreamAudio]->time_base)*packet.pts;
        }
        handle_finished = 1;

        packet.data = FR_pkt_data_org;
        av_free_packet(&packet);

    }while(handle_finished <1);
    ret = pbuf - (uint8_t*)audio_buf;
// LOGE("44444 ret[%d]!!!", ret);
    return  ret;
}

//从视频队列取一个包填充到 FrameVideoRGB.data[0] 所指向的像素矩阵
void ff_play_getvideopkt2display(void *arg){
    AVPacket packet;
    struct SwsContext *img_convert_ctx;
    int handle_finished,frameFinished = 0,ret;
    static int FS_discard_frame=0;

    do{
        if(Flush_Video == 1){
            avcodec_flush_buffers(VideoCodecCtx);
            Flush_Video = 0;
        }
        ret = ff_queue_packet_get(&Queue_Video,&packet,0);
        if(ret<=0){
            break;
        }
        if(packet.stream_index != StreamVideo){
            av_free_packet(&packet);
            continue;
        }
        if(packet.pts != AV_NOPTS_VALUE){
            Clock_Video = av_q2d(FormatCtx->streams[StreamVideo]->time_base)*packet.pts;
        }
        LOGE("Discard flag[%d], PKT_FLAG[%d], clk_diff[%f] clk_video[%f]，head[0x %x %x %x %x %x %x %x]", \
FS_discard_frame, packet.flags & AV_PKT_FLAG_KEY, Clock_Video-Clock_Audio, Clock_Video, \
packet.data[0],packet.data[1],packet.data[2],packet.data[3],packet.data[4],packet.data[5],packet.data[6]);
        if( (StreamAudio >=0 ) && Clock_Video - Clock_Audio > INTERVAL_ALERT){
            if(Clock_Video - Clock_Audio > INTERVAL_ALERT*(Delay_Muti+1) ){
                Delay_Muti++;
            }
            usleep( Delay_Muti * DELAY_BASE * 1000);
            LOGE("DELAY_MUTI[%d] diff[%f]", Delay_Muti, Clock_Video-Clock_Audio);
        }
        if( ( StreamAudio >=0 ) &&  (Clock_Video < Clock_Audio - INTERVAL_MAX) && (packet.flags & AV_PKT_FLAG_KEY)){
            Delay_Muti = 0;
            FS_discard_frame =1;
        }

        if(FS_discard_frame){
            if( (StreamAudio >=0 ) &&  (Clock_Video >= Clock_Audio ) && (packet.flags & AV_PKT_FLAG_KEY)){
                FS_discard_frame =0;
            }
            else if(!(packet.flags & AV_PKT_FLAG_KEY) ){
                av_free_packet(&packet);
                continue;
            }
        }
        avcodec_decode_video2(VideoCodecCtx, FrameVideo, &frameFinished, &packet);
        LOGE("PKT_FLAG[%d] , pic type is[%d]",packet.flags,FrameVideo->pict_type);
        if(frameFinished <=0){
            av_free_packet(&packet);
            continue;
        }
        img_convert_ctx = sws_getContext(VideoCodecCtx->width,
                                         VideoCodecCtx->height, VideoCodecCtx->pix_fmt, VideoCodecCtx->width,
                                         VideoCodecCtx->height, FrameRGB_Pix_Fmt, SWS_BICUBIC, NULL, NULL,
                                         NULL);
        if (img_convert_ctx == NULL) {
            av_free_packet(&packet);
            return;
        }
        sws_scale(img_convert_ctx,
                  (const uint8_t* const *) FrameVideo->data, FrameVideo->linesize,
                  0, VideoCodecCtx->height, FrameVideoRGB->data,
                  FrameVideoRGB->linesize);
        handle_finished = 1;
        av_free_packet(&packet);
    }while(handle_finished <1);
}
//移动流的上下文指针用于快进快退
void ff_play_seek_to(int seek_seconds){
    double  clock;
    int64_t seek_pos;
    int stream_index = -1;
    int seek_flags;

    if (StreamVideo >= 0){
        stream_index = StreamVideo;
        clock = Clock_Video;
    }else if(StreamAudio >= 0){
        stream_index = StreamAudio;
        clock = Clock_Audio;
    }
    seek_pos = (clock + seek_seconds) * AV_TIME_BASE;

    if(stream_index<0)
        return;

    seek_pos= av_rescale_q(seek_pos, AV_TIME_BASE_Q,
                           FormatCtx->streams[stream_index]->time_base);

    seek_flags = seek_seconds < 0 ? AVSEEK_FLAG_BACKWARD : 0;
    LOGE("stream_index[%d] ,seek_pos[%lld] ,seek_flags[%d]",
         stream_index, seek_pos, seek_flags);
    if(av_seek_frame(FormatCtx, stream_index,seek_pos, seek_flags) < 0){
        LOGE("error when seeking!!!!");
    }
}

//读线程
void* ff_play_readpkt_thread(void *arg) {
    AVPacket packet;
    int res=-1;
    double pkt_pts;
    int stream_index= -1;
    int64_t seek_target;

    while ( Isrun_Read >0 ) {
        if(Seek_Time != 0){
            ff_play_seek_to(Seek_Time);
            packet_queue_flush(&Queue_Video);
            packet_queue_flush(&Queue_Audio);
            Flush_Video = 1;
            Flush_Audio = 1;
            LOGE("hahahaha  seektime [%d]!!!!", Seek_Time);
            Seek_Time = 0;
        }
        res = av_read_frame(FormatCtx, &packet);
//LOGE("read frame res[%d] stream_idx[%d]",res ,packet.stream_index);
        if(res < 0) continue;
// Is this a packet from the video stream?
        if (packet.stream_index == StreamVideo) {
            ff_queue_packet_put(&Queue_Video, &packet);
        } else if (packet.stream_index == StreamAudio) {
            ff_queue_packet_put(&Queue_Audio, &packet);
        } else {
            av_free_packet(&packet);
        }
    }
    packet_queue_flush(&Queue_Video);
    packet_queue_flush(&Queue_Audio);
    Isrun_Read = -1;
    return NULL;
}

int ff_play_begin_read_thread(void){
    int res;
    res = pthread_create(&Task_Read, NULL, ff_play_readpkt_thread, NULL);
    Isrun_Read = 1;
    return res;
}
//设置快进快退时间
void ff_play_jump(int second){
    Seek_Time =  second;
}
