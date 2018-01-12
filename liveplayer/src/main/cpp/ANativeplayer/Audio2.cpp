//
// Created by liuxiang on 2017/9/24.
//


#include <pthread.h>
#include "Log.h"
#include "Audio2.h"
#include <unistd.h>


//第一次主动调用在调用线程
//之后在新线程中回调
void bqPlayerCallback(SLAndroidSimpleBufferQueueItf bq, void *context) {
    Audio *audio = (Audio *) context;
//    LOGI("call %d====%d %d", pthread_self(), gettid(),audio);
    int datalen = audio->decodeAudio();
//    LOGI("decode %d", datalen);
    if (datalen > 0) {
        double timer = (double) datalen / (double)(A_SAMPLE_RATE * audio->channels * 2);
        audio->clock += timer;
        (*bq)->Enqueue(bq, audio->buff, datalen);
//        SLresult result;
//        result = (*bq)->Enqueue(bq, audio->buff, datalen);
//        LOGI("  bqPlayerCallback :%d", result);
    } else
        LOGI("decodeAudio error");
}


Audio::Audio() : codec(0), index(-1), clock(0), isPlay(0), engineObject(0), engineEngine(0),
                 outputMixObject(0), bqPlayerObject(0), bqPlayerEffectSend(0), bqPlayerVolume(0),
                 bqPlayerPlay(0), bqPlayerBufferQueue(0), swr_ctx(0) {
    //重采样后的缓冲数据
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);

    buff = (uint8_t *) malloc(MAX_AUDIO_FRME_SIZE * sizeof(uint8_t));
    channels = av_get_channel_layout_nb_channels(A_CH_LAYOUT);
}

Audio::~Audio() {
    if (buff) {
        free(buff);
    }
    pthread_cond_destroy(&cond);
    pthread_mutex_destroy(&mutex);
}

int Audio::createPlayer() {
    SLresult result;
    // 创建引擎engineObject
    result = slCreateEngine(&engineObject, 0, NULL, 0, NULL, NULL);
    if (SL_RESULT_SUCCESS != result) {
        return 0;
    }
    // 实现引擎engineObject
    result = (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
    if (SL_RESULT_SUCCESS != result) {
        return 0;
    }
    // 获取引擎接口engineEngine
    result = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE,
                                           &engineEngine);
    if (SL_RESULT_SUCCESS != result) {
        return 0;
    }
    // 创建混音器outputMixObject
    result = (*engineEngine)->CreateOutputMix(engineEngine, &outputMixObject, 0,
                                              0, 0);
    if (SL_RESULT_SUCCESS != result) {
        return 0;
    }
    // 实现混音器outputMixObject
    result = (*outputMixObject)->Realize(outputMixObject, SL_BOOLEAN_FALSE);
    if (SL_RESULT_SUCCESS != result) {
        return 0;
    }
    result = (*outputMixObject)->GetInterface(outputMixObject, SL_IID_ENVIRONMENTALREVERB,
                                              &outputMixEnvironmentalReverb);
    const SLEnvironmentalReverbSettings settings = SL_I3DL2_ENVIRONMENT_PRESET_DEFAULT;
    if (SL_RESULT_SUCCESS == result) {
        (*outputMixEnvironmentalReverb)->SetEnvironmentalReverbProperties(
                outputMixEnvironmentalReverb, &settings);
    }


    //======================
    SLDataLocator_AndroidSimpleBufferQueue android_queue = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE,
                                                            2};
    SLDataFormat_PCM pcm = {SL_DATAFORMAT_PCM, 2, SL_SAMPLINGRATE_44_1, SL_PCMSAMPLEFORMAT_FIXED_16,
                            SL_PCMSAMPLEFORMAT_FIXED_16,
                            SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT,
                            SL_BYTEORDER_LITTLEENDIAN};
//   新建一个数据源 将上述配置信息放到这个数据源中
    SLDataSource slDataSource = {&android_queue, &pcm};
//    设置混音器
    SLDataLocator_OutputMix outputMix = {SL_DATALOCATOR_OUTPUTMIX, outputMixObject};

    SLDataSink audioSnk = {&outputMix, NULL};
    const SLInterfaceID ids[3] = {SL_IID_BUFFERQUEUE, SL_IID_EFFECTSEND,
            /*SL_IID_MUTESOLO,*/ SL_IID_VOLUME};
    const SLboolean req[3] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE,
            /*SL_BOOLEAN_TRUE,*/ SL_BOOLEAN_TRUE};
    //先讲这个
    (*engineEngine)->CreateAudioPlayer(engineEngine, &bqPlayerObject, &slDataSource,
                                       &audioSnk, 2,
                                       ids, req);
    //初始化播放器
    (*bqPlayerObject)->Realize(bqPlayerObject, SL_BOOLEAN_FALSE);

//    得到接口后调用  获取Player接口
    (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_PLAY, &bqPlayerPlay);

//    注册回调缓冲区 //获取缓冲队列接口
    (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_BUFFERQUEUE,
                                    &bqPlayerBufferQueue);
    //缓冲接口回调
    (*bqPlayerBufferQueue)->RegisterCallback(bqPlayerBufferQueue, bqPlayerCallback, this);
//    获取音量接口
    (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_VOLUME, &bqPlayerVolume);

//    获取播放状态接口
    (*bqPlayerPlay)->SetPlayState(bqPlayerPlay, SL_PLAYSTATE_PLAYING);

    bqPlayerCallback(bqPlayerBufferQueue, this);
    return 1;
}


int Audio::decodeAudio() {
    AVPacket *packet = av_packet_alloc();
    AVFrame *frame = av_frame_alloc();
    int data_size = 0;
    while (1) {
        deQueue(packet);
        if (packet->pts != AV_NOPTS_VALUE) {
            clock = av_q2d(time_base) * packet->pts;
        }
        int ret = avcodec_send_packet(codec, packet);
        if (ret == AVERROR(EAGAIN)) {
            LOGI("avcodec_send_packet audio need more");
            continue;
        } else if (ret < 0) {
            LOGI("audio break");
            break;
        }
        ret = avcodec_receive_frame(codec, frame);
        if (ret == AVERROR(EAGAIN)) {
            LOGI("avcodec_receive_frame audio need more");
            continue;
        } else if (ret < 0) {
            LOGI("audio break");
            break;
        }
        uint64_t dst_nb_samples = av_rescale_rnd(
                swr_get_delay(swr_ctx, frame->sample_rate) + frame->nb_samples,
                frame->sample_rate,
                frame->sample_rate, AVRounding(1));
        int nb = swr_convert(swr_ctx, &buff, dst_nb_samples,
                             (const uint8_t **) frame->data, frame->nb_samples);
        data_size = nb * channels * 2;
        av_packet_unref(packet);
        av_frame_unref(frame);
        break;
    }
    av_packet_free(&packet);
    av_frame_free(&frame);
    return data_size;
}


void Audio::setCodec(AVCodecContext *codec) {
    if (this->codec) {
        if (avcodec_is_open(this->codec))
            avcodec_close(this->codec);
        avcodec_free_context(&this->codec);
        this->codec = 0;
    }
    this->codec = codec;

    swr_ctx = swr_alloc_set_opts(0, A_CH_LAYOUT, A_PCM_BITS, A_SAMPLE_RATE,
                                 codec->channel_layout,
                                 codec->sample_fmt,
                                 codec->sample_rate, 0, 0);
    swr_init(swr_ctx);
}

double Audio::getClock() {
    return clock;
}


void *play_audio(void *args) {

//    LOGI("play_audio %d====%d", pthread_self(), gettid());
    Audio *audio = (Audio *) args;
    audio->createPlayer();
//    while (audio->isPlay)
//        av_usleep(10000);
    pthread_exit(0);

}

void Audio::play() {

//    LOGI("play %d====%d", pthread_self(), gettid());
    isPlay = 1;
    pthread_create(&p_playid, 0, play_audio, this);
}

void Audio::stop() {
    LOGI("AUDIO stop");
    //因为可能卡在 deQueue
    pthread_mutex_lock(&mutex);
    isPlay = 0;
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);
    pthread_join(p_playid, 0);
    if (bqPlayerPlay) {
        (*bqPlayerPlay)->SetPlayState(bqPlayerPlay, SL_PLAYSTATE_STOPPED);
        bqPlayerPlay = 0;
    }
    if (bqPlayerObject) {
        (*bqPlayerObject)->Destroy(bqPlayerObject);
        bqPlayerObject = 0;

        bqPlayerBufferQueue = 0;
        bqPlayerVolume = 0;
    }

    if (outputMixObject) {
        (*outputMixObject)->Destroy(outputMixObject);
        outputMixObject = 0;
    }

    if (engineObject) {
        (*engineObject)->Destroy(engineObject);
        engineObject = 0;
        engineEngine = 0;
    }
    size_t size = queue.size();
    for (int i = 0; i < size; ++i) {
        AVPacket *pkt = queue.front();
        av_packet_free(&pkt);
        queue.pop();
    }
    if (swr_ctx)
        swr_free(&swr_ctx);
    if (this->codec) {
        if (avcodec_is_open(this->codec))
            avcodec_close(this->codec);
        avcodec_free_context(&this->codec);
        this->codec = 0;
    }
    LOGI("AUDIO clear");
}


int Audio::enQueue(const AVPacket *packet) {
    if (!isPlay)
        return 0;
    AVPacket *pkt = av_packet_alloc();
    if (av_packet_ref(pkt, packet) < 0) {
        return 0;
    }
    LOGI("enQueue");
    pthread_mutex_lock(&mutex);
    queue.push(pkt);
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);
    return 1;
}

int Audio::deQueue(AVPacket *packet) {
    int ret = 0;
    pthread_mutex_lock(&mutex);
    while (isPlay) {
        LOGI("dequeue");
        if (!queue.empty()) {
            if (av_packet_ref(packet, queue.front()) < 0) {
                ret = 0;
                break;
            }
            AVPacket *pkt = queue.front();
            queue.pop();
            av_packet_unref(pkt);
            av_packet_free(&pkt);
            ret = 1;
            LOGI("dequeue 2");
            break;
        } else {
            pthread_cond_wait(&cond, &mutex);
        }
    }
    pthread_mutex_unlock(&mutex);
    return ret;
}

