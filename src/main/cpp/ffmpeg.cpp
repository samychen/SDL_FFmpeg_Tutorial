//
// Created by Administrator on 2017/10/17 0017.
//
#include "ffmpeg.h"
#include <android/log.h>
#define LOG_I(...) __android_log_print(ANDROID_LOG_ERROR , "main", __VA_ARGS__)

int nRet = 0;
AVFormatContext* icodec = NULL;
AVInputFormat* ifmt = NULL;
char szError[256];
AVFormatContext* oc = NULL;
AVOutputFormat* ofmt = NULL;
AVStream * video_st = NULL;
AVStream * audio_st = NULL;
AVCodec *audio_codec;
AVCodec *video_codec;
double audio_pts = 0.0;
double video_pts = 0.0;
int video_stream_idx = -1;
int audio_stream_idx = -1;
AVPacket pkt;
AVBitStreamFilterContext * vbsf_aac_adtstoasc = NULL;

//video param
int m_dwWidth = 0;
int m_dwHeight = 0;
double m_dbFrameRate = 25.0;  //帧率
AVCodecID video_codecID = AV_CODEC_ID_H264;
AVPixelFormat video_pixelfromat = AV_PIX_FMT_YUV420P;
char spspps[100];
int spspps_size = 0;

//audio param
int m_dwChannelCount = 2;      //声道
int m_dwBitsPerSample = 16;    //样本
int m_dwFrequency = 44100;     //采样率
AVCodecID audio_codecID = AV_CODEC_ID_AAC;
int audio_frame_size  = 1024;


int InitInput(char * Filename,AVFormatContext ** iframe_c,AVInputFormat** iinputframe)
{
    int i = 0;
    nRet = avformat_open_input(iframe_c, Filename,NULL, NULL);
    if (nRet != 0)
    {
        av_strerror(nRet, szError, 256);
        LOG_I("%s%d",szError,nRet);
        LOG_I("Call avformat_open_input function failed!\n");
        return 0;
    }
    if (avformat_find_stream_info(*iframe_c,NULL) < 0)
    {
        LOG_I("Call av_find_stream_info function failed!\n");
        return 0;
    }
    //输出视频信息
    av_dump_format(*iframe_c, -1, Filename, 0);

    //添加音频信息到输出context
    for (i = 0; i < (*iframe_c)->nb_streams; i++)
    {
        if ((*iframe_c)->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            video_stream_idx = i;
            m_dwWidth = (*iframe_c)->streams[i]->codec->width;
            m_dwHeight = (*iframe_c)->streams[i]->codec->height;
            m_dbFrameRate = av_q2d((*iframe_c)->streams[i]->r_frame_rate);
            video_codecID = (*iframe_c)->streams[i]->codec->codec_id;
            video_pixelfromat = (*iframe_c)->streams[i]->codec->pix_fmt;
            spspps_size = (*iframe_c)->streams[i]->codec->extradata_size;
            memcpy(spspps,(*iframe_c)->streams[i]->codec->extradata,spspps_size);
        }
        else if ((*iframe_c)->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
        {
            audio_stream_idx = i;
            m_dwChannelCount = (*iframe_c)->streams[i]->codec->channels;
            switch ((*iframe_c)->streams[i]->codec->sample_fmt)
            {
                case AV_SAMPLE_FMT_U8:
                    m_dwBitsPerSample  = 8;
                    break;
                case AV_SAMPLE_FMT_S16:
                    m_dwBitsPerSample  = 16;
                    break;
                case AV_SAMPLE_FMT_S32:
                    m_dwBitsPerSample  = 32;
                    break;
                default:
                    break;
            }
            m_dwFrequency = (*iframe_c)->streams[i]->codec->sample_rate;
            audio_codecID = (*iframe_c)->streams[i]->codec->codec_id;
            audio_frame_size = (*iframe_c)->streams[i]->codec->frame_size;
        }
    }
    return 1;
}

int InitOutput()
{
    int i = 0;
    /* allocate the output media context */
    avformat_alloc_output_context2(&oc, NULL, "flv", OUTPUTURL);
    if (!oc)
    {
        return getchar();
    }
    ofmt = oc->oformat;

    /* open the output file, if needed */
    if (!(ofmt->flags & AVFMT_NOFILE))
    {
        if (avio_open(&oc->pb, OUTPUTURL, AVIO_FLAG_WRITE) < 0)
        {
            LOG_I("Could not open '%s'\n", OUTPUTURL);
            return getchar();
        }
    }

    //添加音频信息到输出context
    if(audio_stream_idx != -1)
    {
        ofmt->audio_codec = audio_codecID;
        audio_st = Add_output_stream_2(oc, AVMEDIA_TYPE_AUDIO,audio_codecID,&audio_codec);
    }

    //添加视频信息到输出context
    ofmt->video_codec = video_codecID;
    video_st = Add_output_stream_2(oc, AVMEDIA_TYPE_VIDEO,video_codecID,&video_codec);

    if (OpenCodec(video_st,video_codec) < 0)   //打开视频编码器
    {
        LOG_I("can not open video codec\n");
        return getchar();
    }

    if(audio_stream_idx != -1)
    {
        if (OpenCodec(audio_st,audio_codec) < 0)   //打开音频编码器
        {
            LOG_I("can not open audio codec\n");
            return getchar();
        }
    }

    av_dump_format(oc, 0, OUTPUTURL, 1);

    if (avformat_write_header(oc, NULL))
    {
        LOG_I("Call avformat_write_header function failed.\n");
        return 0;
    }

    if(audio_stream_idx != -1)
    {
        if ((strstr(oc->oformat->name, "flv") != NULL) ||
            (strstr(oc->oformat->name, "mp4") != NULL) ||
            (strstr(oc->oformat->name, "mov") != NULL) ||
            (strstr(oc->oformat->name, "3gp") != NULL))
        {
            if (audio_st->codec->codec_id == AV_CODEC_ID_AAC) //AV_CODEC_ID_AAC
            {
                vbsf_aac_adtstoasc =  av_bitstream_filter_init("aac_adtstoasc");
            }
        }
    }
    if(vbsf_aac_adtstoasc == NULL)
    {
        return -1;
    }
    return 1;
}

AVStream * Add_output_stream_2(AVFormatContext* output_format_context,AVMediaType codec_type_t, AVCodecID codecID,AVCodec **codec)
{
    AVCodecContext* output_codec_context = NULL;
    AVStream * output_stream = NULL;

    /* find the encoder */
    *codec = avcodec_find_encoder(codecID);
    if (!(*codec))
    {
        return NULL;
    }
    output_stream = avformat_new_stream(output_format_context, *codec);
    if (!output_stream)
    {
        return NULL;
    }

    output_stream->id = output_format_context->nb_streams - 1;
    output_codec_context = output_stream->codec;
    output_codec_context->codec_id = codecID;
    output_codec_context->codec_type = codec_type_t;

    switch (codec_type_t)
    {
        case AVMEDIA_TYPE_AUDIO:
            AVRational CodecContext_time_base;
            CodecContext_time_base.num = 1;
            CodecContext_time_base.den = m_dwFrequency;
            output_stream->time_base = CodecContext_time_base;
            output_codec_context->time_base = CodecContext_time_base;
            output_stream->start_time = 0;
            output_codec_context->sample_rate = m_dwFrequency;
            output_codec_context->channels = m_dwChannelCount;
            output_codec_context->frame_size = audio_frame_size;
            switch (m_dwBitsPerSample)
            {
                case 8:
                    output_codec_context->sample_fmt  = AV_SAMPLE_FMT_U8;
                    break;
                case 16:
                    output_codec_context->sample_fmt  = AV_SAMPLE_FMT_S16;
                    break;
                case 32:
                    output_codec_context->sample_fmt  = AV_SAMPLE_FMT_S32;
                    break;
                default:
                    break;
            }
            output_codec_context->block_align = 0;
            if(! strcmp( output_format_context-> oformat-> name,  "mp4" ) ||
               !strcmp (output_format_context ->oformat ->name , "mov" ) ||
               !strcmp (output_format_context ->oformat ->name , "3gp" ) ||
               !strcmp (output_format_context ->oformat ->name , "flv" ))
            {
                output_codec_context->flags |= CODEC_FLAG_GLOBAL_HEADER;
            }
            break;
        case AVMEDIA_TYPE_VIDEO:
            AVRational r_frame_rate_t;
            r_frame_rate_t.num = 1000;
            r_frame_rate_t.den = (int)(m_dbFrameRate * 1000);
            output_stream->time_base = r_frame_rate_t;
            output_codec_context->time_base = r_frame_rate_t;

            AVRational r_frame_rate_s;
            r_frame_rate_s.num = (int)(m_dbFrameRate * 1000);
            r_frame_rate_s.den = 1000;
            output_stream->r_frame_rate = r_frame_rate_s;

            output_stream->start_time = 0;
            output_codec_context->pix_fmt = video_pixelfromat;
            output_codec_context->width = m_dwWidth;
            output_codec_context->height = m_dwHeight;
            output_codec_context->extradata = (uint8_t *)spspps;
            output_codec_context->extradata_size = spspps_size;
            //这里注意不要加头，demux的时候 h264filter过滤器会改变文件本身信息
            //这里用output_codec_context->extradata 来显示缩略图
            //if(! strcmp( output_format_context-> oformat-> name,  "mp4" ) ||
            //	!strcmp (output_format_context ->oformat ->name , "mov" ) ||
            //	!strcmp (output_format_context ->oformat ->name , "3gp" ) ||
            //	!strcmp (output_format_context ->oformat ->name , "flv" ))
            //{
            //	output_codec_context->flags |= CODEC_FLAG_GLOBAL_HEADER;
            //}
            break;
        default:
            break;
    }
    return output_stream;
}

int OpenCodec(AVStream * istream, AVCodec * icodec)
{
    AVCodecContext *c = istream->codec;
    nRet = avcodec_open2(c, icodec, NULL);
    return nRet;
}

int UintInput()
{
    /* free the stream */
    av_free(icodec);
    return 1;
}

int UintOutput()
{
    int i = 0;
    nRet = av_write_trailer(oc);
    if (nRet < 0)
    {
        av_strerror(nRet, szError, 256);
//        printf(szError);
        printf("\n");
        LOG_I("Call av_write_trailer function failed\n");
    }
    if (vbsf_aac_adtstoasc !=NULL)
    {
        av_bitstream_filter_close(vbsf_aac_adtstoasc);
        vbsf_aac_adtstoasc = NULL;
    }
    av_dump_format(oc, -1, OUTPUTURL, 1);
    /* Free the streams. */
    for (i = 0; i < oc->nb_streams; i++)
    {
        av_freep(&oc->streams[i]->codec);
        av_freep(&oc->streams[i]);
    }
    if (!(ofmt->flags & AVFMT_NOFILE))
    {
        /* Close the output file. */
        avio_close(oc->pb);
    }
    av_free(oc);
    return 1;
}

void read_frame(AVFormatContext *oc)
{
    int ret = 0;

    ret = av_read_frame(icodec, &pkt);

    if (pkt.stream_index == video_stream_idx)
    {
        write_frame_3(oc,VEDIO_ID,pkt);
    }
    else if (pkt.stream_index == audio_stream_idx)
    {
        write_frame_3(oc,AUDIO_ID,pkt);
    }
}

void write_frame_3(AVFormatContext *oc,int ID,AVPacket pkt_t)
{
    int64_t pts = 0, dts = 0;
    int nRet = -1;
    AVRational time_base_t;
    time_base_t.num = 1;
    time_base_t.den = 1000;

    if(ID == VEDIO_ID)
    {
        AVPacket videopacket_t;
        av_init_packet(&videopacket_t);

        if (av_dup_packet(&videopacket_t) < 0)
        {
            av_free_packet(&videopacket_t);
        }

        videopacket_t.pts = pkt_t.pts;
        videopacket_t.dts = pkt_t.dts;
        videopacket_t.pos = 0;
//        videopacket_t.priv = 0;
        videopacket_t.flags = 1;
        videopacket_t.convergence_duration = 0;
        videopacket_t.side_data_elems = 0;
        videopacket_t.stream_index = VEDIO_ID;
        videopacket_t.duration = 0;
        videopacket_t.data = pkt_t.data;
        videopacket_t.size = pkt_t.size;
        nRet = av_interleaved_write_frame(oc, &videopacket_t);
        if (nRet != 0)
        {
            LOG_I("error av_interleaved_write_frame _ video\n");
        }
        av_free_packet(&videopacket_t);
    }
    else if(ID == AUDIO_ID)
    {
        AVPacket audiopacket_t;
        av_init_packet(&audiopacket_t);

        if (av_dup_packet(&audiopacket_t) < 0)
        {
            av_free_packet(&audiopacket_t);
        }

        audiopacket_t.pts = pkt_t.pts;
        audiopacket_t.dts = pkt_t.dts;
        audiopacket_t.pos = 0;
//        audiopacket_t.priv = 0;
        audiopacket_t.flags = 1;
        audiopacket_t.duration = 0;
        audiopacket_t.convergence_duration = 0;
        audiopacket_t.side_data_elems = 0;
        audiopacket_t.stream_index = AUDIO_ID;
        audiopacket_t.duration = 0;
        audiopacket_t.data = pkt_t.data;
        audiopacket_t.size = pkt_t.size;

        //添加过滤器
        if(! strcmp( oc-> oformat-> name,  "mp4" ) ||
           !strcmp (oc ->oformat ->name , "mov" ) ||
           !strcmp (oc ->oformat ->name , "3gp" ) ||
           !strcmp (oc ->oformat ->name , "flv" ))
        {
            if (audio_st->codec->codec_id == AV_CODEC_ID_AAC)
            {
                if (vbsf_aac_adtstoasc != NULL)
                {
                    AVPacket filteredPacket = audiopacket_t;
                    int a = av_bitstream_filter_filter(vbsf_aac_adtstoasc,
                                                       audio_st->codec, NULL,&filteredPacket.data, &filteredPacket.size,
                                                       audiopacket_t.data, audiopacket_t.size, audiopacket_t.flags & AV_PKT_FLAG_KEY);
                    if (a >  0)
                    {
                        av_free_packet(&audiopacket_t);
//                        filteredPacket.destruct = av_destruct_packet;
                        audiopacket_t = filteredPacket;
                    }
                    else if (a == 0)
                    {
                        audiopacket_t = filteredPacket;
                    }
                    else if (a < 0)
                    {
                        fprintf(stderr, "%s failed for stream %d, codec %s",
                                vbsf_aac_adtstoasc->filter->name,audiopacket_t.stream_index,audio_st->codec->codec ?  audio_st->codec->codec->name : "copy");
                        av_free_packet(&audiopacket_t);

                    }
                }
            }
        }
        nRet = av_interleaved_write_frame(oc, &audiopacket_t);
        if (nRet != 0)
        {
            LOG_I("error av_interleaved_write_frame _ audio\n");
        }
        av_free_packet(&audiopacket_t);
    }
}

int Es2Mux_2()
{
    for (;;)
    {
        read_frame(oc);
    }
    return 1;
}


