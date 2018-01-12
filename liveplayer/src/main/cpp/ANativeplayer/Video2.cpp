//
// Created by liuxiang on 2017/9/24.
//


#include <pthread.h>
#include "Video2.h"

#include "Log.h"

typedef struct {
    Audio *audio;
    Video *video;
} Sync;

static const double SYNC_THRESHOLD = 0.01;
static const double NOSYNC_THRESHOLD = 10.0;

void default_video_call(AVFrame *frame) {

}

static void (*video_call)(AVFrame *frame) = default_video_call;

Video::Video() : clock(0), codec(0), index(-1),
                 isPlay(0), frame_timer(0) {
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);
}

Video::~Video() {
    pthread_cond_destroy(&cond);
    pthread_mutex_destroy(&mutex);
}

void Video::setPlayCall(void (*call)(AVFrame *frame)) {
    video_call = call;
}

void Video::setCodec(AVCodecContext *codec) {
    if (this->codec) {
        if (avcodec_is_open(this->codec))
            avcodec_close(this->codec);
        avcodec_free_context(&this->codec);
        this->codec = 0;
    }
    this->codec = codec;
}

double Video::synchronize(AVFrame *frame, double pts) {
    //clock是当前播放的时间位置
    if (pts != 0)
        clock = pts;
    else //pst为0 则先把pts设为上一帧时间
        pts = clock;
    //可能有pts为0 则主动增加clock
    //frame->repeat_pict = 当解码时，这张图片需要要延迟多少
    //需要求出扩展延时：
    //extra_delay = repeat_pict / (2*fps) 显示这样图片需要延迟这么久来显示
    double repeat_pict = frame->repeat_pict;
    //使用AvCodecContext的而不是stream的
    double frame_delay = av_q2d(codec->time_base);
    //如果time_base是1,25 把1s分成25份，则fps为25
    //fps = 1/(1/25)
    double fps = 1 / frame_delay;
    //pts 加上 这个延迟 是显示时间
    double extra_delay = repeat_pict / (2 * fps);
    double delay = extra_delay + frame_delay;
//    LOGI("extra_delay:%f",extra_delay);
    clock += delay;
    return pts;
}

void *play_video(void *args) {
    Sync *sync = (Sync *) args;

    //转换rgba
    SwsContext *sws_ctx = sws_getContext(
            sync->video->codec->width, sync->video->codec->height, sync->video->codec->pix_fmt,
            sync->video->codec->width, sync->video->codec->height, AV_PIX_FMT_RGBA,
            SWS_BILINEAR, 0, 0, 0);

    AVFrame *rgb_frame = av_frame_alloc();
    int out_size = av_image_get_buffer_size(AV_PIX_FMT_RGBA, sync->video->codec->width,
                                            sync->video->codec->height, 1);
    uint8_t *out_buffer = (uint8_t *) malloc(sizeof(uint8_t) * out_size);
    av_image_fill_arrays(rgb_frame->data, rgb_frame->linesize, out_buffer,
                         AV_PIX_FMT_RGBA,
                         sync->video->codec->width, sync->video->codec->height, 1);
    double last_pts;
    double last_delay;
    AVPacket *packet = av_packet_alloc();
    AVFrame *frame = av_frame_alloc();
    double pts, delay, audio_clock, diff, sync_threshold, actual_delay;
    while (sync->video->isPlay) {
        sync->video->deQueue(packet);
        if (!sync->video->frame_timer)
            sync->video->frame_timer = (double) av_gettime() / 1000000.0;
        int ret = avcodec_send_packet(sync->video->codec, packet);
        if (ret == AVERROR(EAGAIN)) {
            goto cont;
        } else if (ret < 0) {
            break;
        }
        ret = avcodec_receive_frame(sync->video->codec, frame);
        if (ret == AVERROR(EAGAIN)) {
            goto cont;
        } else if (ret < 0) {
            break;
        }
        if ((pts = av_frame_get_best_effort_timestamp(frame)) == AV_NOPTS_VALUE)
            pts = 0;
        //pts 单位就是time_base
        //av_q2d转为双精度浮点数 x pts 得到pts---显示时间:秒
        pts *= av_q2d(sync->video->time_base);
        pts = sync->video->synchronize(frame, pts);
        sws_scale(sws_ctx,
                  (const uint8_t *const *) frame->data, frame->linesize, 0, frame->height,
                  rgb_frame->data, rgb_frame->linesize);
        delay = pts - last_pts;
        //当前帧与上个帧的显示时间差
        if (delay <= 0 || delay >= 1.0) {
            //如果延误不正确 太高或太低，使用上一个
            //确保这个延迟有意义
            delay = last_delay;
        }
        audio_clock = sync->audio->getClock();
//        LOGI("ref_clock %f", audio_clock);
//        LOGI("current pts %f", pts);
//
//        LOGI("delay %f", delay);
        last_delay = delay;
        last_pts = pts;

        //大于0就是视频快了,小于就是音频快了
        diff = sync->video->clock - audio_clock;
//        LOGI("diff %f", diff);
        //两帧时间差
        sync_threshold = (delay > SYNC_THRESHOLD) ? delay : SYNC_THRESHOLD;
        //防止没有音频 如果误差在10以内则进行同步，否则认为没音频
        //音视频间隔在10s内才处理
        if (fabs(diff) < NOSYNC_THRESHOLD) {
            if (diff <= -sync_threshold) {
                delay = 0;
//                LOGI("加快速度 %f", delay);
            } else if (diff >= sync_threshold) {
                delay = 2 * delay;
//                LOGI("减慢速度 %f", delay);
            }
        }
        //得到当前帧应该显示的时间
        sync->video->frame_timer += delay;
        //当前帧应该显示的时间减去当前时间 就是需要等待的时间
        actual_delay = sync->video->frame_timer - (av_gettime() / 1000000.0);
        //让最小刷新时间是10ms
        if (actual_delay < 0.010) {
            actual_delay = 0.010;
        }
//        LOGI("sleep %f", actual_delay * 1000000);
        av_usleep(actual_delay * 1000000);
        video_call(rgb_frame);
        cont:
        av_packet_unref(packet);
        av_frame_unref(frame);
    }
    sync->video->frame_timer = 0;
    av_packet_free(&packet);
    av_frame_free(&frame);
    av_frame_free(&rgb_frame);
    sws_freeContext(sws_ctx);
    size_t size = sync->video->queue.size();
    for (int i = 0; i < size; ++i) {
        AVPacket *pkt = sync->video->queue.front();
        av_packet_free(&pkt);
        sync->video->queue.pop();
    }
    free(sync);
    LOGI("VIDEO EXIT");
    pthread_exit(0);
}


void Video::play(Audio *audio) {
    Sync *sync = (Sync *) malloc(sizeof(Sync));
    sync->audio = audio;
    sync->video = this;
    isPlay = 1;
    pthread_create(&p_playid, 0, play_video, sync);
}

void Video::stop() {

    LOGI("VIDEO stop");

    pthread_mutex_lock(&mutex);
    isPlay = 0;
    //因为可能卡在 deQueue
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);

    pthread_join(p_playid, 0);
    LOGI("VIDEO join pass");
    if (this->codec) {
        if (avcodec_is_open(this->codec))
            avcodec_close(this->codec);
        avcodec_free_context(&this->codec);
        this->codec = 0;
    }
    LOGI("VIDEO close");
}


int Video::enQueue(const AVPacket *packet) {
    if (!isPlay)
        return 0;
    AVPacket *pkt = av_packet_alloc();
    if (av_packet_ref(pkt, packet) < 0)
        return 0;
    pthread_mutex_lock(&mutex);
    queue.push(pkt);
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);
    return 1;
}

int Video::deQueue(AVPacket *packet) {
    int ret = 0;
    pthread_mutex_lock(&mutex);
    while (isPlay) {
        if (!queue.empty()) {
            if (av_packet_ref(packet, queue.front()) < 0) {
                ret = false;
                break;
            }
            AVPacket *pkt = queue.front();
            queue.pop();
            av_packet_unref(pkt);
            av_packet_free(&pkt);
            ret = 1;
            break;
        } else {
            pthread_cond_wait(&cond, &mutex);
        }
    }
    pthread_mutex_unlock(&mutex);
    return ret;
}