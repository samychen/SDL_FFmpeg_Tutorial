//
// Created by Administrator on 2017/11/5 0005.
//
#ifndef __FF_QUEUE_H__
#define __FF_QUEUE_H__

#include <pthread.h>
#include "libavformat/avformat.h"

typedef struct PacketQueue {
    AVPacketList *first_pkt, *last_pkt;
    int nb_packets;
    int size;
    pthread_mutex_t mutex;
    pthread_cond_t  cond;
} PacketQueue;

void packet_queue_init(PacketQueue *q);

int packet_queue_put(PacketQueue *q, AVPacket *pkt);

int packet_queue_get(PacketQueue *q, AVPacket *pkt, int block);

void packet_queue_flush(PacketQueue *q);
#endif
