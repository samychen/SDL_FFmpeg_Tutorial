#ifndef __DATE__H__
#define __DATE__H__

#include <inttypes.h>
#include <math.h>
#include <limits.h>
#include <signal.h>
#include "libavutil/avstring.h"
//#include "libavutil/colorspace.h"
#include "libavutil/mathematics.h"
#include "libavutil/pixdesc.h"
#include "libavutil/imgutils.h"
#include "libavutil/dict.h"
#include "libavutil/log.h"
#include "libavutil/parseutils.h"
#include "libavutil/samplefmt.h"
#include "libavutil/avassert.h"
#include "libavformat/avformat.h"
//#include "libavdevice/avdevice.h"
#include "libswscale/swscale.h"
#include "libavutil/opt.h"
#include "libavcodec/avfft.h"
#include "libswresample/swresample.h"

#ifdef ANDROID
#include <android/log.h>
#include <SDL.h>
#include <SDL_thread.h>
#include <SDL_events.h>

#define TAG "ffplay"
#define LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, TAG, __VA_ARGS__)
#define  Android_Notify(x)   do { LOGV("Noitfy:%d", x);}while(0)
#else
#include <SDL/SDL.h>
#include <SDL/SDL_thread.h>
#include <SDL/SDL_events.h>
#define  LOGV(...) printf(__VA_ARGS__)
#define  Android_Notify(x)   do { printf("Noitfy:%d", x);}while(0)
#endif


#define MAX_QUEUE_SIZE (15 * 1024 * 1024)
//Min Frame is 10sec frame
#define MIN_FRAMES (10*15)
//Max Frame is 3min video frame
#define MAX_FRAMES (3*60*15)


#define MSG_LOAD_FINISHED 		10
#define MSG_LOAD_UNFINISHED     11
#define MSG_OPEN_ERROR			12
#define MSG_OPEN_OK				13

#define RET_OK 					0
#define RET_INIT_ERROR			-1
#define RET_PREPARE_ERROR		-2
#define RET_FILE_NOTEXIST		-3
#define RET_NETWORK_ERROR		-4
#define RET_FILE_OPENERROR		-5
#define RET_FORMAT_NOT_SUPPORT	-6
#define RET_STREAM_OPEN_FAIL	-7

#define SKIP_DELAY_TIME			0.3

/* SDL audio buffer size, in samples. Should be small to have precise
A/V sync as SDL does not have hardware buffer fullness info. */
#define SDL_AUDIO_BUFFER_SIZE 1024

/* no AV sync correction is done if below the AV sync threshold */
#define AV_SYNC_THRESHOLD 0.01
/* no AV correction is done if too big error */
#define AV_NOSYNC_THRESHOLD 10.0

/* maximum audio speed change to get correct sync */
#define SAMPLE_CORRECTION_PERCENT_MAX 10

/* we use about AUDIO_DIFF_AVG_NB A-V differences to make the average */
#define AUDIO_DIFF_AVG_NB   20

/* NOTE: the size must be big enough to compensate the hardware audio buffersize size */
#define SAMPLE_ARRAY_SIZE (2 * 65536)
#define SDL_EVENTMASK(X)    (1<<(X))

#define AVCODEC_MAX_AUDIO_FRAME_SIZE 192000

#define VIDEO_PICTURE_QUEUE_SIZE 2
#define SUBPICTURE_QUEUE_SIZE 4



enum {
	AV_SYNC_AUDIO_MASTER, /* default choice */
	AV_SYNC_VIDEO_MASTER, AV_SYNC_EXTERNAL_CLOCK, /* synchronize to an external clock */
};

enum ShowMode {
	SHOW_MODE_NONE = -1,
	SHOW_MODE_VIDEO = 0,
	SHOW_MODE_WAVES,
	SHOW_MODE_RDFT,
	SHOW_MODE_NB
};

#define FF_ALLOC_EVENT   (SDL_USEREVENT)
#define FF_REFRESH_EVENT (SDL_USEREVENT + 1)
#define FF_QUIT_EVENT    (SDL_USEREVENT + 2)
#define KU_PLAYER_SEEK   (SDL_USEREVENT	+ 3)
#define KU_PLAYER_PAUSE  (SDL_USEREVENT	+ 4)
#define KU_PLAYER_BUFCHK (SDL_USEREVENT	+ 5)


typedef struct PacketQueue {
	AVPacketList *first_pkt, *last_pkt;
	int nb_packets;
	int size;
	int abort_request;
	SDL_mutex *mutex;
	SDL_cond *cond;
} PacketQueue;


typedef struct VideoPicture {
	double pts;                    ///< presentation time stamp for this picture
	double duration;                         ///< expected duration of the frame
	int64_t pos;                                 ///< byte position in file
	int skip;

#ifdef SDL12
	SDL_Overlay *bmp;
#else
	AVFrame *pFrameRGB;
	int numBytes;
	uint8_t *buffer;
#endif

	int width, height; /* source height & width */
	AVRational sample_aspect_ratio;
	int allocated;
	int reallocate;
	enum AVSampleFormat pix_fmt;

} VideoPicture;


typedef struct VideoState {
	SDL_Thread *read_tid;
	SDL_Thread *video_tid;
	SDL_Thread *refresh_tid;
	AVInputFormat *iformat;
	int no_background;
	int abort_request;
	int force_refresh;
	int paused;
	int last_paused;
	int seek_req;
	int seek_flags;
	int64_t seek_pos;
	int64_t seek_rel;
	int read_pause_return;
	AVFormatContext *ic;

	int audio_stream;

	int av_sync_type;
	double external_clock; /* external clock base */
	int64_t external_clock_time;

	double audio_clock;
	double audio_diff_cum; /* used for AV difference average computation */
	double audio_diff_avg_coef;
	double audio_diff_threshold;
	int audio_diff_avg_count;
	AVStream *audio_st;
	PacketQueue audioq;
	int audio_hw_buf_size;
	DECLARE_ALIGNED(16,uint8_t,audio_buf2) [AVCODEC_MAX_AUDIO_FRAME_SIZE * 4];
	uint8_t silence_buf[SDL_AUDIO_BUFFER_SIZE];
	uint8_t *audio_buf;
	uint8_t *audio_buf1;
	unsigned int audio_buf_size; /* in bytes */
	int audio_buf_index; /* in bytes */
	int audio_write_buf_size;
	AVPacket audio_pkt_temp;
	AVPacket audio_pkt;
	enum AVSampleFormat audio_src_fmt;
	enum AVSampleFormat audio_tgt_fmt;
	int audio_src_channels;
	int audio_tgt_channels;
	int64_t audio_src_channel_layout;
	int64_t audio_tgt_channel_layout;
	int audio_src_freq;
	int audio_tgt_freq;
	struct SwrContext *swr_ctx;
	double audio_current_pts;
	double audio_current_pts_drift;
	int frame_drops_early;
	int frame_drops_late;
	AVFrame *frame;

	enum ShowMode show_mode;
	int16_t sample_array[SAMPLE_ARRAY_SIZE];
	int sample_array_index;
	int last_i_start;
	RDFTContext *rdft;
	int rdft_bits;
	FFTSample *rdft_data;
	int xpos;

	double frame_timer;
	double frame_last_pts;
	double frame_last_duration;
	double frame_last_dropped_pts;
	double frame_last_returned_time;
	double frame_last_filter_delay;
	int64_t frame_last_dropped_pos;
	double video_clock; ///< pts of last decoded frame / predicted pts of next decoded frame
	int video_stream;
	AVStream *video_st;
	PacketQueue videoq;
	double video_current_pts; ///< current displayed pts (different from video_clock if frame fifos are used)
	double video_current_pts_drift; ///< video_current_pts - time (av_gettime) at which we updated video_current_pts - used to have running video pts
	int64_t video_current_pos;                   ///< current displayed file pos
	VideoPicture pictq[VIDEO_PICTURE_QUEUE_SIZE];
	int pictq_size, pictq_rindex, pictq_windex;
	SDL_mutex *pictq_mutex;
	SDL_cond *pictq_cond;
	struct SwsContext *img_convert_ctx;

	char filename[1024];
	int width, height, xleft, ytop;
	int step;

	int refresh;
	int last_video_stream, last_audio_stream, last_subtitle_stream;
	int load;
} VideoState;

typedef struct AllocEventProps {
	VideoState *is;
	AVFrame *frame;
} AllocEventProps;

#endif
