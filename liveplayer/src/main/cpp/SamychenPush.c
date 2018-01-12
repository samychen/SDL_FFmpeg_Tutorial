//
// Created by Administrator on 2017/12/30 0030.
//
#include <jni.h>

#include <android/log.h>
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/time.h"
#define LOG_I(...) __android_log_print(ANDROID_LOG_ERROR , "main", __VA_ARGS__)
#define LOG_E(...) __android_log_print(ANDROID_LOG_ERROR , "main", __VA_ARGS__)
extern  int isPlay;

//Output FFmpeg's av_log()
void custom_log(void *ptr, int level, const char* fmt, va_list vl){

    //To TXT file
    FILE *fp=fopen("/storage/emulated/0/av_log.txt","a+");
    if(fp){
        vfprintf(fp,fmt,vl);
        fflush(fp);
        fclose(fp);
    }

    //To Logcat
    //LOGE(fmt, vl);
}

JNIEXPORT jint JNICALL Java_com_samychen_gracefulwrapper_liveplayer_StreamActivity_stream
        (JNIEnv *env, jobject ins, jstring inputurl_, jstring outputurl_) {
    const char *inputurl = (*env)->GetStringUTFChars(env,inputurl_, 0);
    const char *outputurl = (*env)->GetStringUTFChars(env,outputurl_, 0);
    AVFormatContext *inFmtCtx = NULL,*outFmtCtx = NULL;
    av_log_set_callback(custom_log);
    av_register_all();
    avformat_network_init();
    int ret = 0;
    if ((ret=avformat_open_input(&inFmtCtx,inputurl,0,0))<0){
        LOG_E("%s","无法打开文件");
        goto end;
    }
    if((ret=avformat_find_stream_info(inFmtCtx,0))<0){
        LOG_E("%s","获取文件信息失败");
        goto end;
    }
    //         |---------------------------------|
    //FLV格式：|header || frame|| frame || ....  |
    //         |---------------------------------|
    //推送flv格式文件
    avformat_alloc_output_context2(&outFmtCtx,NULL,"flv",outputurl);
    if (!outFmtCtx) {
        LOG_I( "Could not create output context\n");
        ret = AVERROR_UNKNOWN;
        goto end;
    }
    //根据输入的AVStream，构造输出的AVStream，如采用率,编码器等信息
    int i =0;//nb_streams可能包含音频，视频，字幕流
    for (;  i<inFmtCtx->nb_streams ; i++) {
        AVStream *in_stream = inFmtCtx->streams[i];
        AVStream *out_stream = avformat_new_stream(outFmtCtx,in_stream->codec->codec);
        if (!out_stream) {
            LOG_I( "Failed allocating output stream\n");
            ret = AVERROR_UNKNOWN;
            goto end;
        }
        ret = avcodec_copy_context(out_stream->codec,in_stream->codec);
        if (ret < 0) {
            LOG_I( "Failed to copy context from input to output stream codec context\n");
            goto end;
        }
        if (outFmtCtx->oformat->flags == AVFMT_GLOBALHEADER){
            out_stream->codec->flags = CODEC_FLAG_GLOBAL_HEADER;
        }
    }
    //打开输出AVIOContext IO流上下文
    AVOutputFormat *ofmt = outFmtCtx->oformat;
    if (!(ofmt->flags & AVFMT_NOFILE)){
        ret = avio_open(&outFmtCtx->pb,outputurl,AVIO_FLAG_WRITE);
        if (ret < 0) {
            LOG_I( "Could not open output URL '%s'", outputurl);
            goto end;
        }
    }
    //先写一个头
    ret = avformat_write_header(outFmtCtx,NULL);
    if (ret<0){
        LOG_E("推流发生错误\n");
        goto end;
    }
    //获取视频流的索引位置
    int videoindex = -1;
    for (i = 0; i < inFmtCtx->nb_streams; i++) {
        if (inFmtCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO){
            videoindex = i;
            break;
        }
    }
    int frame_index = 0;
    int64_t  start_time = av_gettime();
    AVPacket pkt;
    isPlay = 1;
    while (isPlay){
        AVStream *in_stream,*out_stream;
        ret = av_read_frame(inFmtCtx,&pkt);
        if (ret<0){
            break;
        }
        //raw data裸流处理(如h264格式，没有封装),裸流没有pts和dts
        if (pkt.pts==AV_NOPTS_VALUE){
            //write pts
            AVRational time_base1=inFmtCtx->streams[videoindex]->time_base;
            int64_t  calc_duration=(double)AV_TIME_BASE/av_q2d(inFmtCtx->streams[videoindex]->time_base);
            pkt.pts=(double)(frame_index*calc_duration)/(double)(av_q2d(time_base1)*AV_TIME_BASE);
            pkt.dts=pkt.pts;
            pkt.duration=(double)calc_duration/(double)(av_q2d(time_base1)*AV_TIME_BASE);
        }
        if (pkt.stream_index==videoindex){
            //ffmpeg 读取速度很快，减轻流媒体的负担
            AVRational time_base=inFmtCtx->streams[videoindex]->time_base;
            AVRational time_base_q={1,AV_TIME_BASE};
            int64_t  pts_time = av_rescale_q(pkt.dts,time_base,time_base_q);
            int64_t  now_time = av_gettime()-start_time;
            if (pts_time>now_time){
                //延迟 ,慢慢读
                av_usleep(pts_time-now_time);
            }
        }
        in_stream = inFmtCtx->streams[pkt.stream_index];
        out_stream = outFmtCtx->streams[pkt.stream_index];
        //copy packet
        pkt.pts=av_rescale_q_rnd(pkt.pts,in_stream->time_base,out_stream->time_base,AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
        pkt.dts=av_rescale_q_rnd(pkt.pts,in_stream->time_base,out_stream->time_base,AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
        pkt.duration=av_rescale_q(pkt.duration,in_stream->time_base,out_stream->time_base);
        pkt.pos=-1;
        if (pkt.stream_index==videoindex){
            LOG_I("第%d帧",frame_index);
            frame_index++;
        }
        //写数据
        ret = av_interleaved_write_frame(outFmtCtx,&pkt);
        if (ret<0){
            LOG_E("Error muxing packet");
            break;
        }
        av_free_packet(&pkt);
    }
    isPlay = 0;
    av_write_trailer(outFmtCtx);
    goto end;
    end:
    avformat_free_context(inFmtCtx);
    avformat_free_context(outFmtCtx);
    (*env)->ReleaseStringUTFChars(env, inputurl_, inputurl);
    (*env)->ReleaseStringUTFChars(env, outputurl_, outputurl);
}
JNIEXPORT void JNICALL Java_com_samychen_gracefulwrapper_liveplayer_StreamActivity_stop
        (JNIEnv *env, jobject ins) {
    if (isPlay) {
        isPlay = 0;
    }
}

