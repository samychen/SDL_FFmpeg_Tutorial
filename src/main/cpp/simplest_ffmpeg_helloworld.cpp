/*
 * file: simplest_ffmpeg_helloworld.cpp
 * Simplest FFmpeg HelloWorld
 *
 * Author Lei Xiaohua
 * Communication University of China / Digital TV Technology
 * http://blog.csdn.net/leixiaohua1020
 * http://blog.csdn.net/leixiaohua1020/article/details/46889849
 *
 * modify openswc
 * http://blog.csdn.net/openswc
 *  
 * gcc simplest_ffmpeg_helloworld.cpp -I./../ffmpeg_build/include/ -L./../ffmpeg_build/lib/ -lavfilter -lavformat -lavcodec -lswresample -lswscale -lpostproc -lavutil -o sffhellow
 * gcc simplest_ffmpeg_helloworld.cpp -I./ffmpeg_build/include/ -L./ffmpeg_build/lib/ -lavfilter -lavformat -lavcodec -lswresample -lswscale -lpostproc -lavutil -pthread -lva -lm -lz -o sffhellow
 * 
*/

#include <stdio.h>
#include <string.h>

#define __STDC_CONSTANT_MACROS

//Linux...
#ifdef __cplusplus
extern "C"
{
#endif
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>
#ifdef __cplusplus
};
#endif

/*
 * Protocol Support Information
*/
void urlprotocolinfo(char* info)
{
	const char* buff = NULL;

	struct URLProtocol *pup = NULL;	
	struct URLProtocol **p_temp = &pup;
	//struct URLProtocol **p_temp;
	//p_temp = &pup;
	
	//printf("%x %x %x %x\n", p_temp, *p_temp, &(*p_temp), &pup);

	av_register_all();

	//Input
#if 0
	avio_enum_protocols((void **)p_temp, 0);
	while ((*p_temp) != NULL)
	{
		sprintf(info, "%s[In ][%10s]\n", info, avio_enum_protocols((void **)p_temp, 0));
	}
#else
	do
	{
		buff = avio_enum_protocols((void **)p_temp, 0);
		if(NULL != buff)
		{
			strcat(info, "[In ][");
			strcat(info, buff);
			strcat(info, "]\n");
		}
	} while(NULL != buff);
#endif

	pup = NULL;
	//*p_temp = NULL;
	//Output
#if 0
	avio_enum_protocols((void **)p_temp, 1);
	while ((*p_temp) != NULL)
	{
		sprintf(info, "%s[Out][%10s]\n", info, avio_enum_protocols((void **)p_temp, 1));
	}
#else
	do
	{
		buff = avio_enum_protocols((void **)p_temp, 1);
		if(NULL != buff)
		{
			strcat(info, "[Out][");
			strcat(info, buff);
			strcat(info, "]\n");
		}
	} while(NULL != buff);
#endif

	return ;
}

/*
 * AVFormat Support Information
*/
void avformatinfo(char* info)
{
	//char buff[32] = {0};
	AVInputFormat *if_temp = NULL;
	AVOutputFormat *of_temp = NULL;

	av_register_all();

#if 0
	AVInputFormat *if_temp = av_iformat_next(NULL);
	AVOutputFormat *of_temp = av_oformat_next(NULL);

	//Input
	while(if_temp!=NULL)
	{
		sprintf(info, "%s[In ] %10s\n", info, if_temp->name);
		if_temp=if_temp->next;
	}

	//Output
	while (of_temp != NULL)
	{
		sprintf(info, "%s[Out] %10s\n", info, of_temp->name);
		of_temp = of_temp->next;
	}
#else
	//Input
	do 
	{
		if_temp = av_iformat_next(if_temp);
		if(NULL != if_temp)
		{
			strcat(info, "[In][");
			//sprintf(buff, "%10s", if_temp->name);
			//strcat(info, buff);
			strcat(info, if_temp->name);
			strcat(info, "]\n");
		}
	} while(NULL != if_temp);

	//Output
	do 
	{
		of_temp = av_oformat_next(of_temp);
		if(NULL != of_temp)
		{
			strcat(info, "[Out][");
			strcat(info, of_temp->name);
			strcat(info, "]\n");
		}
	} while(NULL != of_temp);
#endif

	return ;
}

void avcodecprint(int isdec, int type, char* str)
{
	AVCodec *c_temp = NULL;

	do
	{
		c_temp = av_codec_next(c_temp);
		if(NULL != c_temp)
		{		
			switch(c_temp->type)
			{
				case AVMEDIA_TYPE_VIDEO:
					if((1 == isdec) && (NULL != c_temp->decode) && (type == c_temp->type))
					{
						strcat(str, "[Dec]");
						strcat(str, "[Video][");
						strcat(str, c_temp->name);
						strcat(str, "]\n");							
					}
					if((0 == isdec) && (NULL == c_temp->decode) && (type == c_temp->type))
					{
						strcat(str, "[Enc]");
						strcat(str, "[Video][");
						strcat(str, c_temp->name);
						strcat(str, "]\n");	
					}						
					break;
				case AVMEDIA_TYPE_AUDIO:
					if((1 == isdec) && (NULL != c_temp->decode) && (type == c_temp->type))
					{
						strcat(str, "[Dec]");	
						strcat(str, "[Audio][");
						strcat(str, c_temp->name);
						strcat(str, "]\n");							
					}
					if((0 == isdec) && (NULL == c_temp->decode) && (type == c_temp->type))
					{
						strcat(str, "[Enc]");
						strcat(str, "[Audio][");
						strcat(str, c_temp->name);
						strcat(str, "]\n");	
					}
					break;
				default:
					if((1 == isdec) && (NULL != c_temp->decode) && (-1 == type))
					{
						strcat(str, "[Dec]");	
						strcat(str, "[Other][");
						strcat(str, c_temp->name);
						strcat(str, "]\n");							
					}
					if((0 == isdec) && (NULL == c_temp->decode) && (-1 == type))
					{
						strcat(str, "[Enc]");
						strcat(str, "[Other][");
						strcat(str, c_temp->name);
						strcat(str, "]\n");	
					}
					break;	
			}
		}		
	} while(NULL != c_temp);

	return ;
}

/*
 * AVCodec Support Information
*/
void avcodecinfo(char* info)
{
	av_register_all();

#if 0
	AVCodec *c_temp = av_codec_next(NULL);

	while(c_temp!=NULL)
	{
		if (c_temp->decode!=NULL)
		{
			sprintf(info, "%s[Dec]", info);
		}
		else
		{
			sprintf(info, "%s[Enc]", info);
		}

		switch (c_temp->type)
		{
			case AVMEDIA_TYPE_VIDEO:
				sprintf(info, "%s[Video]", info);
				break;
			case AVMEDIA_TYPE_AUDIO:
				sprintf(info, "%s[Audio]", info);
				break;
			default:
				sprintf(info, "%s[Other]", info);
				break;
		}

		sprintf(info, "%s %10s\n", info, c_temp->name);

		c_temp=c_temp->next;
	}
#else
	//Dec: Video
	//avcodecprint(1, AVMEDIA_TYPE_VIDEO, info);

	//Dec: Audio
	//avcodecprint(1, AVMEDIA_TYPE_AUDIO, info);

	//Dec: Other
	//avcodecprint(1, -1, info);

	//Enc: Video
	//avcodecprint(0, AVMEDIA_TYPE_VIDEO, info);

	//Enc: Audio
	//avcodecprint(0, AVMEDIA_TYPE_AUDIO, info);

	//Enc: Other
	avcodecprint(0, -1, info);
#endif

	return ;
}

/*
 * AVFilter Support Information
*/
void avfilterinfo(char* info)
{
	const AVFilter *f_temp = NULL;

	avfilter_register_all();

#if 0
	AVFilter *f_temp = (AVFilter *)avfilter_next(NULL);
	
	while (f_temp != NULL)
	{
		sprintf(info, "%s[%15s]\n", info, f_temp->name);
		f_temp=f_temp->next;
	}
#else
	do 
	{
		f_temp = avfilter_next(f_temp);
		if(NULL != f_temp)
		{
			strcat(info, "[");
			//sprintf(buff, "%10s", f_temp->name);
			//strcat(info, buff);
			strcat(info, f_temp->name);
			strcat(info, "]\n");
		}
	} while(NULL != f_temp);
#endif

	return ;
}

/*
 * Configuration Information
*/
void configurationinfo(char* info)
{
	av_register_all();

	sprintf(info, "%s\n", avcodec_configuration());

}

int main(int argc, char* argv[])
{
	char *infostr = (char *)malloc(40000);

#if 1	
	memset(infostr, 0, 40000);
	configurationinfo(infostr);
	printf("\n<<Configuration>>\n%s\n", infostr);
#endif

#if 0
	memset(infostr, 0, 40000);
	urlprotocolinfo(infostr);
	printf("\n<<URLProtocol>>\n%s\n", infostr);
#endif

#if 0
	memset(infostr, 0, 40000);
	avformatinfo(infostr);
	printf("\n<<AVFormat>>\n%s", infostr);
#endif

#if 0
	memset(infostr, 0, 40000);
	avcodecinfo(infostr);
	printf("\n<<AVCodec>>\n%s", infostr);
#endif

#if 1
	memset(infostr, 0, 40000);
	avfilterinfo(infostr);
	printf("\n<<AVFilter>>\n%s",infostr);
#endif

	free(infostr);

	return 0;
}
