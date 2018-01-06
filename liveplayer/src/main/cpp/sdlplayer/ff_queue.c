//
// Created by Administrator on 2017/11/5 0005.
//
#include <stdio.h>
#include <pthread.h>
#include "ff_queue.h"

int ff_queue_quit = 0;
//队列初始化
void ff_queue_init(PacketQueue *q) {
    memset(q, 0, sizeof(PacketQueue));
}

void ff_queue_set_quit(void){
    ff_queue_quit=1;
}
//向队列放一个包
int ff_queue_packet_put(PacketQueue *q, AVPacket *pkt) {
    AVPacketList *pkt1;
    if (av_dup_packet(pkt) < 0) {
        return -1;
    }
    pkt1 = av_malloc(sizeof(AVPacketList));
    if (!pkt1)
        return -1;
    pkt1->pkt = *pkt;
    pkt1->next = NULL;
    pthread_mutex_lock(&q->mutex);
    if (!q->last_pkt)
        q->first_pkt = pkt1;
    else
        q->last_pkt->next = pkt1;
    q->last_pkt = pkt1;
    q->nb_packets++;
    q->size += pkt1->pkt.size;
    pthread_mutex_unlock(&q->mutex);
    return 0;
}
//从队列取一个包
int ff_queue_packet_get(PacketQueue *q, AVPacket *pkt, int block) {
    AVPacketList *pkt1;
    int ret;
    pthread_mutex_lock(&q->mutex);
    for (;;) {
        if (ff_queue_quit) {
            ret = -1;
            break;
        }
        pkt1 = q->first_pkt;
        if (pkt1) {
            q->first_pkt = pkt1->next;
            if (!q->first_pkt)
                q->last_pkt = NULL;
            q->nb_packets--;
            q->size -= pkt1->pkt.size;
            *pkt = pkt1->pkt;
            av_free(pkt1);
            ret = 1;
            break;
        } else if (!block) {
            ret = 0;
            break;
        } else {
            pthread_cond_wait(&q->cond, &q->mutex);
        }
    }
    pthread_mutex_unlock(&q->mutex);
    return ret;
}


void packet_queue_flush(PacketQueue *q) {
    AVPacketList *pkt, *pkt1;
    pthread_mutex_lock(&q->mutex);
    for(pkt = q->first_pkt; pkt != NULL; pkt = pkt1) {
        pkt1 = pkt->next;
        av_free_packet(&pkt->pkt);
        av_freep(&pkt);
    }
    q->last_pkt = NULL;
    q->first_pkt = NULL;
    q->nb_packets = 0;
    q->size = 0;
    pthread_mutex_unlock(&q->mutex);
}
