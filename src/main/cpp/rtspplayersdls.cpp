/*
 * gcc rtspplayersdls.cpp -I./../ffmpeg_build/include/ -I./../../SDL2-2.0.5/build/include/ -L./../ffmpeg_build/lib/ -L./../../SDL2-2.0.5/build/lib/ -lavdevice -lavfilter -lavformat -lavcodec -lswresample -lswscale -lpostproc -lavutil -pthread -lva -lm -lz -lSDL2 -lSDL2main -g -o rtspplayers
 * gcc rtspplayersdls.cpp $(pkg-config --cflags --libs libavformat libavcodec libswscale libavutil sdl2) -g -o rtspplayers
 * gcc rtspimg.c -I./ffmpeg_build/include/ -L./ffmpeg_build/lib/ -lavdevice -lavfilter -lavformat -lavcodec -lswresample -lswscale -lpostproc -lavutil -pthread -lva -lm -lz -o rtspimg
*/

#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>

#define __STDC_CONSTANT_MACROS

//Linux...
#ifdef __cplusplus
extern "C"
{
#endif
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include "SDL.h"
#ifdef __cplusplus
};
#endif

//Refresh Event
#define SFM_REFRESH_EVENT  (SDL_USEREVENT + 1)

#define SFM_BREAK_EVENT  (SDL_USEREVENT + 2)

int thread_exit=0;
int thread_pause=0;

int sfp_refresh_thread(void *opaque)
{
	thread_exit=0;
	thread_pause=0;

	while (!thread_exit)
	{
		if(!thread_pause)
		{
			SDL_Event event;
			event.type = SFM_REFRESH_EVENT;
			SDL_PushEvent(&event);
		}
		SDL_Delay(40);
	}
	thread_exit=0;
	thread_pause=0;
	//Break
	SDL_Event event;
	event.type = SFM_BREAK_EVENT;
	SDL_PushEvent(&event);

	return 0;
}

static void pgm_save(unsigned char *buf, int wrap, int xsize, int ysize, char *filename)
{
    FILE *f;
    int i;

    f = fopen(filename,"w");
    fprintf(f, "P5\n%d %d\n%d\n", xsize, ysize, 255);
    for (i = 0; i < ysize; i++)
        fwrite(buf + i * wrap, 1, xsize, f);
    fclose(f);
}

/*
 * ffmpeg -loglevel 56 -rtsp_transport udp -i rtsp://192.168.1.88 -r 1/10 -f image2 -strftime 1 /home/wr/temp/img%Y-%m-%d_%H-%M-%S.png
*/
int main(int argc, char **argv)
{
    AVFormatContext *pFormatCtx;
    AVCodecContext *pCodecCtx;
    AVCodec *pCodec;
    AVFrame *pFrame, *pFrameYUV;
    AVPacket *packet;
    int i, videoindex;
    uint8_t * out_buffer;
    int ret, got_picture;

	struct SwsContext *img_convert_ctx;

    int ffmpeg_log_level = 0;

    const char *rtsp_url="rtsp://192.168.1.87";

	//------------SDL----------------
	int screen_w,screen_h;
	SDL_Window *screen; 
	SDL_Renderer* sdlRenderer;
	SDL_Texture* sdlTexture;
	SDL_Rect sdlRect;
	SDL_Thread *video_tid;
	SDL_Event event;

	struct timeval ts;
	int iTimestamp = 0;
	long lTimes = 0;

    AVDictionary *avdic=NULL;

    char option_key[32]="rtsp_transport";
    char option_value[16]="udp"; // tcp, udp
    //char option_key2[32]="max_delay";
    //char option_value2[16]="5000000";

	if((2 != argc) || (0 == strcmp("help", argv[1])))
	{
		printf("Usage: \n");
		printf("  rtspimg help --- help information\n");
		printf("  rtspimg img ---save image from video stream\n");
		return 0;
	}

    ffmpeg_log_level = av_log_get_level();
    printf("ffmpeg_log_level is %d.\n", ffmpeg_log_level);
    av_log_set_level(AV_LOG_ERROR); //AV_LOG_TRACE //AV_LOG_DEBUG //AV_LOG_INFO //AV_LOG_ERROR
	ffmpeg_log_level = av_log_get_level();
	printf("av_log_set_level is *%d*\n", ffmpeg_log_level);

    av_register_all();
    avformat_network_init();

    pFormatCtx = avformat_alloc_context();

    /* set rtsp_transport protocol: tcp or udp.*/
    av_dict_set(&avdic, option_key, option_value, 0);
    /* set max_delay value: us.*/
    //av_dict_set(&avdic,option_key2,option_value2,0);
    /* set -r(framerate) 1/5 */
	//av_dict_set(&avdic, "framerate", "1/5", 0);
    /* -strftime 1 */
    //av_dict_set(&avdic, "strftime", "true", 0);
	
	//iformat = av_find_input_format("image2");

    //open video/audio data stream,and read head file info
    //if (avformat_open_input(&pFormatCtx, filename, NULL, NULL) != 0)
    //if (avformat_open_input(&pFormatCtx, rtsp_url, iformat, &avdic) != 0)
	if (avformat_open_input(&pFormatCtx, rtsp_url, NULL, &avdic) != 0)
    {
        printf("Couldn't open input stream.\n");
        return -1;
    }

	//get a frame of decode data stream
    if (avformat_find_stream_info(pFormatCtx, NULL)<0)
    {
        printf("Couldn't find stream information.\n");
        return -2;
    }

    //find video stream channel
    videoindex = -1;
    for(i = 0; i< pFormatCtx->nb_streams; i++)
    {
        if(pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            videoindex = i;
            break;
        }
    }

    if(videoindex == -1)
    {
        printf("Didn't find a video stream.\n");
        return -3;
    }

    //get video stream decode info
    pCodecCtx = pFormatCtx->streams[videoindex]->codec;

    //find video decoder
    pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
    if(pCodec == NULL)
    {
        printf("Codec not found.\n");
        return -4;
    }
    //open decoder
    if(avcodec_open2(pCodecCtx, pCodec, NULL)<0)
    {
        printf("Could not open codec.\n");
        return -5;
    }

    printf("file width:%d,height:%d\n", pCodecCtx->width, pCodecCtx->height);
    // allocate a frame memory
    pFrame = av_frame_alloc();
    pFrameYUV = av_frame_alloc();
    //out_buffer = (uint8_t *)av_malloc(avpicture_get_size(AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height));
	out_buffer = (uint8_t *)av_malloc(av_image_get_buffer_size(AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height, 1));
    //avpicture_fill((AVPicture *)pFrameYUV, out_buffer, AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height);
	av_image_fill_arrays(pFrameYUV->data, pFrameYUV->linesize, out_buffer, AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height, 1);

    av_dump_format(pFormatCtx, 0, rtsp_url, 0);
    img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,
    pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);

	if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) 
	{  
		printf( "Could not initialize SDL - %s\n", SDL_GetError()); 
		return -1;
	} 

	//SDL 2.0 Support for multiple windows
	screen_w = pCodecCtx->width;
	screen_h = pCodecCtx->height;

	screen = SDL_CreateWindow("RTSP player", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, screen_w, screen_h,SDL_WINDOW_OPENGL);
	if(!screen) 
	{  
		printf("SDL: could not create window - exiting:%s\n",SDL_GetError());  
		return -1;
	}
	sdlRenderer = SDL_CreateRenderer(screen, -1, 0); 
 
	//IYUV: Y + U + V  (3 planes)
	//YV12: Y + V + U  (3 planes)
	sdlTexture = SDL_CreateTexture(sdlRenderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, pCodecCtx->width, pCodecCtx->height);  

	sdlRect.x = 0;
	sdlRect.y = 0;
	sdlRect.w = screen_w;
	sdlRect.h = screen_h;

	packet=(AVPacket *)av_malloc(sizeof(AVPacket));

	video_tid = SDL_CreateThread(sfp_refresh_thread,NULL,NULL);
	
	gettimeofday(&ts, NULL);
	printf("[%ld.%03ld]av_read_frame:start.\n", ts.tv_sec, ts.tv_usec / 1000);
	lTimes = ts.tv_sec;
	i = 0;

	//------------SDL End------------
	//Event Loop	
	for (;;) 
	{
		//Wait
		SDL_WaitEvent(&event);
		if(event.type == SFM_REFRESH_EVENT)
		{		
			while(1)
			{
				if(av_read_frame(pFormatCtx, packet)<0)
					thread_exit=1;

				if(packet->stream_index==videoindex)
					break;
			}

            ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
            if(ret < 0)
            {
                printf("Decode Error.\n");
                return -1;
            }
            if(got_picture)
            {
                sws_scale(img_convert_ctx, (const uint8_t * const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height, pFrameYUV->data, pFrameYUV->linesize);
				//SDL---------------------------
				SDL_UpdateTexture( sdlTexture, NULL, pFrameYUV->data[0], pFrameYUV->linesize[0] );  
				SDL_RenderClear( sdlRenderer );  
				//SDL_RenderCopy( sdlRenderer, sdlTexture, &sdlRect, &sdlRect );  
				SDL_RenderCopy( sdlRenderer, sdlTexture, NULL, NULL);  
				SDL_RenderPresent( sdlRenderer );  
				//SDL End-----------------------
				if(0 == (iTimestamp % 5))
				{
					i++;
				}
				else
				{
					i = 0;
				}
				if((0 == strcmp("img", argv[1])) && (1 == pFrame->key_frame))
                {
		            //i++;
		            char buf[32] = {0};				
					snprintf(buf, sizeof(buf), "img-rtsp""%d"".pgm", i + (iTimestamp / 5));
					pgm_save(pFrameYUV->data[0], pFrameYUV->linesize[0], pCodecCtx->width, pCodecCtx->height, buf);
					printf("iTimestamp is %d\n", iTimestamp);
                }
            }	 
			gettimeofday(&ts, NULL);
			iTimestamp = ts.tv_sec - lTimes;
			//av_free_packet(packet);
			av_packet_unref(packet);
		}
		else if(event.type==SDL_KEYDOWN)
		{
			//Pause
			if(event.key.keysym.sym == SDLK_SPACE)
				thread_pause=!thread_pause;
		}
		else if(event.type == SDL_QUIT)
		{
			thread_exit=1;
		}
		else if(event.type == SFM_BREAK_EVENT)
		{
			break;
		}
	}

    sws_freeContext(img_convert_ctx);

	SDL_Quit();
	//--------------

    av_frame_free(&pFrameYUV);
    av_frame_free(&pFrame);
    avcodec_close(pCodecCtx);
    avformat_close_input(&pFormatCtx);

	gettimeofday(&ts, NULL);
    printf("[%ld.%03ld]Decode finish and exit.\n", ts.tv_sec, ts.tv_usec / 1000);

    return 0;
}
