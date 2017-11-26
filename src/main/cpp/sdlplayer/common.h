#ifndef  __COMMON_H
#define __COMMON_H

#include "data.h"


/* packet queue handling */
void packet_queue_init(PacketQueue *q) ;

int packet_queue_put_private(PacketQueue *q, AVPacket *pkt) ;

int packet_queue_put(PacketQueue *q, AVPacket *pkt) ;
/* return < 0 if aborted, 0 if no packet and > 0 if packet.  */
int packet_queue_get(PacketQueue *q, AVPacket *pkt, int block) ;

void packet_queue_abort(PacketQueue *q);

void packet_queue_flush(PacketQueue *q) ;

void packet_queue_destroy(PacketQueue *q) ;

void packet_queue_start(PacketQueue *q) ;

#endif
