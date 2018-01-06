#include <jni.h>
#include <android/log.h>


#define LOG_I(...) __android_log_print(ANDROID_LOG_ERROR , "main", __VA_ARGS__)
#define MAX_AUDIO_FRAME_SIZE 192000
#include "SDL.h"
#include "SDL_log.h"
#include "SDL_main.h"

////avcodec:编解码(最重要的库)
//#include "libavcodec/avcodec.h"
////avformat:封装格式处理
//#include "libavformat/avformat.h"
////avutil:工具库(大部分库都需要这个库的支持)
//#include "libavutil/imgutils.h"
////swscale:视频像素数据格式转换
//#include "libswscale/swscale.h"
////导入音频采样数据格式转换库
//#include "libswresample/swresample.h"

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/imgutils.h"
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"
}
#define MAX_AUDIO_FRAME_SIZE 19200

//int main(int argc, char *argv[]) {
//    //第一步：初始化SDL多媒体框架->SDL_Init
//    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) == -1) {
//        LOG_I("SDL_Init failed %s", SDL_GetError());
//        return 0;
//    }
//    LOG_I("SDL_Init Success!");
//    //第二步：初始化SDL窗口
//    //参数一：窗口名称->要求必需是UTF-8编码
//    //参数二：窗口在屏幕上面X坐标
//    //参数三：窗口在屏幕上面Y坐标
//    //参数四：窗口在屏幕上面宽
//    int width = 640;
//    //参数五：窗口在屏幕上面高
//    int height = 352;
//    //参数六：窗口状态(打开的状态:SDL_WINDOW_OPENGL)
//    SDL_Window* sdl_window = SDL_CreateWindow("Dream开车",
//                                              SDL_WINDOWPOS_CENTERED,
//                                              SDL_WINDOWPOS_CENTERED,
//                                              width ,
//                                              height,
//                                              SDL_WINDOW_OPENGL);
//    if (sdl_window == NULL){
//        LOG_I("窗口创建失败");
//        return 0;
//    }
//
//    //第三步：创建渲染器->渲染窗口(OpenGL ES)
//    //最新一期VIP课程
//    //参数一：渲染目标窗口
//    //参数二：从哪里开始渲染(-1:默认从第一个为止开始)
//    //参数三：渲染类型
//    //SDL_RENDERER_SOFTWARE:软件渲染
//    //...
//    SDL_Renderer* sdl_renderer = SDL_CreateRenderer(sdl_window, -1, 0);
//
//    //第四步：创建纹理
//    //参数一：纹理目标渲染器
//    //参数二：渲染格式
//    //参数三：绘制方式(SDL_TEXTUREACCESS_STREAMING:频繁绘制)
//    //参数四：纹理宽
//    //参数五：纹理高
//    SDL_Texture * sdl_texture = SDL_CreateTexture(sdl_renderer,
//                                                  SDL_PIXELFORMAT_IYUV,
//                                                  SDL_TEXTUREACCESS_STREAMING,
//                                                  width,
//                                                  height);
//
//    //第五步：设置纹理数据->播放YUV视频
//    //着色器语言(着色器)、渲染器、纹理等等...
//    //第一点：打开YUV文件(手机：)
//    FILE* yuv_file = fopen("/storage/emulated/0/DreamTestFile/Test.yuv","rb+");
//    if (yuv_file == NULL){
//        LOG_I("文件打开失败");
//        return 0;
//    }
//
//    //第二点：循环读取YUV视频像素数据格式每一帧画面->渲染->设置纹理数据
//    //定义缓冲区(内存空间开辟多大?)
//    //Y:U:V = 4 : 1 : 1
//    //假设：Y = 1.0  U = 0.25  V = 0.25
//    //宽度：Y + U + V = 1.5
//    //换算：Y + U + V = width * height * 1.5
//    char buffer_pix[width * height * 3 / 2];
//
//    //定义渲染器区域
//    SDL_Rect sdl_rect;
//    while (true){
//        //一行一行的读取
//        fread(buffer_pix, 1, width * height * 3 / 2, yuv_file);
//        //判定是否读取完毕
//        if (feof(yuv_file)){
//            break;
//        }
//
//        //设置纹理数据
//        //参数一：目标纹理对象
//        //参数二：渲染区域(NULL:表示默认屏幕窗口宽高)
//        //参数三：视频像素数据
//        //参数四：帧画面宽
//        SDL_UpdateTexture(sdl_texture, NULL, buffer_pix, width);
//
//        //第六步：将纹理数据拷贝到渲染器
//        sdl_rect.x = 0;
//        sdl_rect.y = 0;
//        sdl_rect.w = width;
//        sdl_rect.h = height;
//
//        //先清空
//        SDL_RenderClear(sdl_renderer);
//        //再渲染
//        SDL_RenderCopy(sdl_renderer,sdl_texture,NULL,&sdl_rect);
//
//        //第七步：显示帧画面
//        SDL_RenderPresent(sdl_renderer);
//
//        //第八步：延时渲染(没渲染一帧间隔时间)
//        SDL_Delay(20);
//    }
//
//
//    //第九步：是否内存
//    fclose(yuv_file);
//
//    SDL_DestroyTexture(sdl_texture);
//
//    SDL_DestroyRenderer(sdl_renderer);
//
//
//    //第十步：推出SDL程序
//    SDL_Quit();
//    return 0;
//}
//Buffer:
//|-----------|-------------|
//chunk-------pos---len-----|
static  Uint8  *audio_chunk;
static  Uint32  audio_len;
static  Uint8  *audio_pos;



/* Audio Callback
 * @param stream: A pointer to the audio buffer to be filled
 * @param len: The length (in bytes) of the audio buffer
*/
void  fill_audio(void *udata,Uint8 *stream,int len){
    //SDL 2.0,SDL2和SDL1.x关于视频方面的API差别很大。但是SDL2和SDL1.x关于音频方面的API是一模一样的
    //唯独在回调函数中，SDL2有一个地方和SDL1.x不一样：SDL2中必须首先使用SDL_memset()将stream中的数据设置为0
//    注意：mp3为什么播放不顺畅？
//    len=4096;audio_len=4608;两个相差512！为了这512，还得再调用一次回调函数。。。
//    m4a,aac就不存在此问题(都是4096)！
    SDL_memset(stream, 0, len);
    LOG_I("len=%d",len);
    if(audio_len==0)
        return;
    len=(len>audio_len?audio_len:len);
    SDL_MixAudio(stream,audio_pos,len,SDL_MIX_MAXVOLUME);
    audio_pos += len;
    audio_len -= len;
}
struct SwrContext *au_convert_ctx;

int main(int argc, char *argv[]) {
    //rtmp://202.117.80.19:1935/live/live2
    //http://live.hkstv.hk.lxdns.com/live/hks/playlist.m3u8
    const char *cinputFilePath = "rtmp://v1.one-tv.com/live/mpegts.stream";
    const char *configuration = avcodec_configuration();
    __android_log_print(ANDROID_LOG_INFO,"main","%s",configuration);
    //第一步：注册所有组件
    av_register_all();
    avformat_network_init();
    //第二步：打开视频输入文件
    //参数一：封装格式上下文->AVFormatContext->包含了视频信息(视频格式、大小等等...)
    AVFormatContext *avformat_context = avformat_alloc_context();
    //参数二：打开文件(入口文件)->url
    int avformat_open_result = avformat_open_input(&avformat_context, cinputFilePath, NULL, NULL);
    if (avformat_open_result != 0) {
        //获取异常信息
        char *error_info;
        av_strerror(avformat_open_result, error_info, 1024);
        __android_log_print(ANDROID_LOG_INFO, "main", "异常信息：%s", error_info);
        return 0;
    }
    //第三步：查找视频文件信息
    //参数一：封装格式上下文->AVFormatContext
    //参数二：配置
    //返回值：0>=返回OK，否则失败
    int avformat_find_stream_info_result = avformat_find_stream_info(avformat_context, NULL);
    if (avformat_find_stream_info_result < 0) {
        //获取失败
        char *error_info;
        av_strerror(avformat_find_stream_info_result, error_info, 1024);
        __android_log_print(ANDROID_LOG_INFO, "main", "异常信息：%s", error_info);
        return 0;
    }
    //第四步：查找解码器
    //第一点：获取当前解码器是属于什么类型解码器->找到了视频流
    //音频解码器、视频解码器、字幕解码器等等...
    //获取视频解码器流引用->指针
    int av_stream_index = -1;
    for (int i = 0; i < avformat_context->nb_streams; ++i) {
        //循环遍历每一流
        //视频流、音频流、字幕流等等...
        if (avformat_context->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            //找到了
            av_stream_index = i;
            break;
        }
    }
    if (av_stream_index == -1) {
        __android_log_print(ANDROID_LOG_INFO, "main", "%s", "没有找到视频流");
        return 0;
    }
    int audioStream=-1;
    for(int i=0; i < avformat_context->nb_streams; i++)
        //原为codec_type==CODEC_TYPE_AUDIO
        if(avformat_context->streams[i]->codec->codec_type==AVMEDIA_TYPE_AUDIO) {
            audioStream=i;
            break;
        }
    if(audioStream==-1) {
        __android_log_print(ANDROID_LOG_INFO, "main", "%s", "没有找到音频流");
        return -1;
    }
    //第二点：根据视频流->查找到视频解码器上下文->视频压缩数据
    AVCodecContext *avcodec_context = avformat_context->streams[av_stream_index]->codec;
    AVCodecContext *pCodecCtx=avformat_context->streams[audioStream]->codec;
    //第三点：根据解码器上下文->获取解码器ID
    AVCodec *avcodec = avcodec_find_decoder(avcodec_context->codec_id);
    if (avcodec == NULL) {
        __android_log_print(ANDROID_LOG_INFO, "main", "%s", "没有找到视频解码器");
        return 0;
    }
    AVCodec * pCodec=avcodec_find_decoder(pCodecCtx->codec_id);
    if(pCodec==NULL) {
        __android_log_print(ANDROID_LOG_INFO, "main", "%s", "没有找到音频解码器");
        return -1;
    }
    //第五步：打开解码器
    int avcodec_open2_result = avcodec_open2(avcodec_context, avcodec, NULL);
    if (avcodec_open2_result != 0) {
        char *error_info;
        av_strerror(avcodec_open2_result, error_info, 1024);
        __android_log_print(ANDROID_LOG_INFO, "main", "异常信息：%s", error_info);
        return 0;
    }
    int avcodec_open2_result2 = avcodec_open2(pCodecCtx, pCodec, NULL);
    if(avcodec_open2_result2<0) {
        char *error_info;
        av_strerror(avcodec_open2_result2, error_info, 1024);
        __android_log_print(ANDROID_LOG_INFO, "main", "异常信息3：%s", error_info);
        return -1;
    }
    //输出视频信息
    //输出：文件格式
    __android_log_print(ANDROID_LOG_INFO, "main", "文件格式：%s", avformat_context->iformat->name);
    //输出：解码器名称
    __android_log_print(ANDROID_LOG_INFO, "main", "解码器名称：%s", avcodec->name);
    //第六步：循环读取视频帧，进行循环解码->输出YUV420P视频->格式：yuv格式
    //读取帧数据换成到哪里->缓存到packet里面
    AVPacket *av_packet = (AVPacket *) av_malloc(sizeof(AVPacket));
    //输入->环境一帧数据->缓冲区->类似于一张图
    AVFrame *av_frame_in = av_frame_alloc();
    //输出->帧数据->视频像素数据格式->yuv420p
    AVFrame *av_frame_out_yuv420p = av_frame_alloc();
    //解码的状态类型(0:表示解码完毕，非0:表示正在解码)
    int av_decode_result, current_frame_index = 0;
    AVPacket *packet=(AVPacket *)malloc(sizeof(AVPacket));
    av_init_packet(packet);
    AVFrame *pFrame_a = av_frame_alloc();
    //只有指定了AVFrame的像素格式、画面大小才能真正分配内存
    //缓冲区
    //作用：计算音频/视频占用的字节数，开辟对应的内存空间
    //参数一：缓冲区格式
    //参数二：缓冲区宽度
    //参数三：缓冲区高度
    //参数四：字节对齐(设置通用1)
    int image_size = av_image_get_buffer_size(avcodec_context->pix_fmt, avcodec_context->width, avcodec_context->height,1);
    //开辟缓存空间
    uint8_t *frame_buffer_out = (uint8_t *)av_malloc(image_size);
    //对开辟的缓存空间指定填充数据格式
    //参数一：数据
    //参数二：行数
    //参数三：缓存区
    //参数四：格式
    //参数五：宽度
    //参数六：高度
    //参数七：字节对齐(设置通用1)
    av_image_fill_arrays(av_frame_out_yuv420p->data, av_frame_out_yuv420p->linesize,frame_buffer_out,
                         avcodec_context->pix_fmt,avcodec_context->width, avcodec_context->height,1);

    SwsContext *sws_context = sws_getContext(avcodec_context->width,
                                             avcodec_context->height,
                                             avcodec_context->pix_fmt,
                                             avcodec_context->width,
                                             avcodec_context->height,
                                             avcodec_context->pix_fmt,
                                             SWS_BICUBIC, NULL, NULL, NULL);
    SwrContext* swr_context = swr_alloc();
    int out_ch_layout = AV_CH_LAYOUT_STEREO;
    //参数三：输出音频采样数据格式(说白了：采样精度)
    AVSampleFormat av_sample_format = AV_SAMPLE_FMT_S16;
    //参数四：输出音频采样数据->采样率
    int out_sample_rate = pCodecCtx->sample_rate;//pCodecCtx_a->sample_rate;
    //参数五：输入声道布局类型(立体声、环绕、室内等等...)->默认格式
    int64_t in_ch_layout = av_get_default_channel_layout(pCodecCtx->channels);
    //参数六：输入音频采样数据格式(说白了：采样精度)
    AVSampleFormat in_sample_fmt = pCodecCtx->sample_fmt;
    //参数七：输入音频采样数据->采样率
    int in_sample_rate = pCodecCtx->sample_rate;//不能高于out_sample_rate
    swr_alloc_set_opts(swr_context, out_ch_layout, av_sample_format, out_sample_rate, in_ch_layout,
                       in_sample_fmt, in_sample_rate, 0, NULL);
    swr_init(swr_context);
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) == -1) {
        LOG_I("SDL_Init failed %s", SDL_GetError());
        return 0;
    }
    LOG_I("SDL_Init Success!");
    SDL_AudioSpec wanted_spec;
    wanted_spec.freq = pCodecCtx->sample_rate;//可以为44100
    wanted_spec.format = AUDIO_S16SYS;
    wanted_spec.channels = pCodecCtx->channels;
    wanted_spec.silence = 0;
    wanted_spec.samples = pCodecCtx->frame_size; //播放AAC，M4a，缓冲区的大小
    //wanted_spec.samples = 1152; //播放MP3，WMA时候用
    wanted_spec.callback = fill_audio;
    wanted_spec.userdata = pCodecCtx;
    LOG_I("samples=%d",wanted_spec.samples);
    LOG_I("Audio Spec size=%d",wanted_spec.size);
    //打开音频设备
    if (SDL_OpenAudio(&wanted_spec, NULL)<0){
        LOG_I("can't open audio.\n");
        return -1;
    }
    SDL_Window *sdl_window = SDL_CreateWindow("Dream开车",
                                              SDL_WINDOWPOS_CENTERED,
                                              SDL_WINDOWPOS_CENTERED,
                                              avcodec_context->width,
                                              avcodec_context->height,
                                              SDL_WINDOW_OPENGL);
    if (sdl_window == NULL) {
        LOG_I("窗口创建失败");
        return 0;
    }
    //第三步：创建渲染器->渲染窗口(OpenGL ES)
    //参数一：渲染目标窗口
    //参数二：从哪里开始渲染(-1:默认从第一个为止开始)
    //参数三：渲染类型
    //SDL_RENDERER_SOFTWARE:软件渲染
    SDL_Renderer *sdl_renderer = SDL_CreateRenderer(sdl_window, -1, 0);
    //第四步：创建纹理
    //参数一：纹理目标渲染器
    //参数二：渲染格式
    //参数三：绘制方式(SDL_TEXTUREACCESS_STREAMING:频繁绘制)
    //参数四：纹理宽
    //参数五：纹理高
    SDL_Texture *sdl_texture = SDL_CreateTexture(sdl_renderer,
                                                 SDL_PIXELFORMAT_IYUV,
                                                 SDL_TEXTUREACCESS_STREAMING,
                                                 avcodec_context->width,
                                                 avcodec_context->height);
    SDL_Rect sdl_rect;
    sdl_rect.x = 0;
    sdl_rect.y = 0;
    sdl_rect.w = avcodec_context->width;
    sdl_rect.h = avcodec_context->height;

    uint32_t ret,len = 0;
    int got_picture;
    int index = 0;
    int out_nb_samples=pCodecCtx->frame_size;
    uint8_t *out_buffer;
    int out_channels =av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO);
    int out_buffer_size = av_samples_get_buffer_size(NULL,out_channels,out_nb_samples,AV_SAMPLE_FMT_S16,1);
    size_t size=MAX_AUDIO_FRAME_SIZE*2;
    out_buffer = (uint8_t *)av_malloc(size);
    int64_t in_channel_layout;
    in_channel_layout = av_get_default_channel_layout(pCodecCtx->channels);
    AVSampleFormat out_sample_fmt = AV_SAMPLE_FMT_S16;//输出格式S16
    au_convert_ctx = swr_alloc();
    au_convert_ctx= swr_alloc_set_opts(au_convert_ctx,AV_CH_LAYOUT_STEREO,out_sample_fmt,out_sample_rate,
                                       in_channel_layout,pCodecCtx->sample_fmt,pCodecCtx->sample_rate,0,NULL);
    swr_init(au_convert_ctx);
    int return_num;
    while (av_read_frame(avformat_context, av_packet) >= 0) {
        if (av_packet->stream_index == av_stream_index) {
            avcodec_send_packet(avcodec_context, av_packet);
            //接收一帧数据->解码一帧
            av_decode_result = avcodec_receive_frame(avcodec_context, av_frame_in);
            //解码出来的每一帧数据成功之后，将每一帧数据保存为YUV420格式文件类型(.yuv文件格式)
            if (av_decode_result == 0) {
                sws_scale(sws_context,
                          (const uint8_t *const *) av_frame_in->data,
                          av_frame_in->linesize,
                          0,
                          avcodec_context->height,
                          av_frame_out_yuv420p->data,
                          av_frame_out_yuv420p->linesize);
                SDL_UpdateTexture(sdl_texture, NULL, av_frame_out_yuv420p->data[0],
                                  av_frame_out_yuv420p->linesize[0]);
                SDL_RenderClear(sdl_renderer);
                SDL_RenderCopy(sdl_renderer, sdl_texture, NULL, &sdl_rect);
                SDL_RenderPresent(sdl_renderer);
                SDL_Delay(20);
                current_frame_index++;
            }
        }else if (av_packet->stream_index==audioStream){
            ret = avcodec_decode_audio4( pCodecCtx, pFrame_a,
                                         &got_picture, av_packet);
            if ( ret < 0 )  {// if error len = -1
                LOG_I("Error in decoding audio frame.\n");
                exit(0);
            }
            if ( got_picture > 0 ) {
                return_num  = swr_convert(au_convert_ctx,&out_buffer,MAX_AUDIO_FRAME_SIZE,
                                          (const uint8_t **)pFrame_a->data, pFrame_a->nb_samples);
                out_buffer_size = return_num * (pCodecCtx->channels) * av_get_bytes_per_sample(out_sample_fmt);
//                fwrite(pFrame_a->data[0], 1, pFrame_a->linesize[0], pFile);
                index++;
            }
            LOG_I("samples=%d",wanted_spec.samples);
            LOG_I("pFrame_a->nb_samples=%d",pFrame_a->nb_samples);
            LOG_I("out_buffer_size=%d",out_buffer_size);
//            if(wanted_spec.samples!=pFrame_a->nb_samples){//MP3、AAC不同的samples，重定位
//                SDL_CloseAudio();
//                out_nb_samples = pFrame_a->nb_samples;
//                out_buffer_size=av_samples_get_buffer_size(NULL,out_channels,out_nb_samples,out_sample_fmt,1);
//                wanted_spec.samples=out_nb_samples;
//                SDL_OpenAudio(&wanted_spec,NULL);
//            }
            LOG_I("samples=%d",wanted_spec.samples);
            while(audio_len>0) {//等待直到音频数据播放完毕!
                SDL_Delay(1);
            }
            //设置音频数据缓冲,PCM数据
            audio_chunk = (Uint8 *)out_buffer;
            //设置音频数据长度
            audio_len = out_buffer_size;
            LOG_I("audio_len=%d,pFrame_a->linesize[0]=%d",audio_len,(Uint32) pFrame_a->linesize[0]);
            //audio_len = 4096;
            //播放mp3的时候改为audio_len = 4096
            //则会比较流畅，但是声音会变调！MP3一帧长度4608
            //使用一次回调函数（4096字节缓冲）播放不完，所以还要使用一次回调函数，导致播放缓慢。。。
            //设置初始播放位置
            audio_pos = audio_chunk;
            SDL_PauseAudio(0);
            //回放音频数据

            //printf("don't close, audio playing...\n");

        }
        av_free_packet(av_packet);
    }
    return_num = swr_convert(au_convert_ctx, &out_buffer, MAX_AUDIO_FRAME_SIZE, NULL, 0);
    out_buffer_size = return_num * (pCodecCtx->channels) * av_get_bytes_per_sample(out_sample_fmt);
    SDL_CloseAudio();//关闭音频设备
//    fclose(pFile);
    avcodec_close(pCodecCtx);
    SDL_DestroyTexture(sdl_texture);
    SDL_DestroyRenderer(sdl_renderer);
    //第十步：推出SDL程序
    SDL_Quit();
    av_packet_free(&av_packet);
    av_frame_free(&av_frame_in);
    av_frame_free(&av_frame_out_yuv420p);
    avcodec_close(avcodec_context);
    avformat_free_context(avformat_context);
    return 0;
}