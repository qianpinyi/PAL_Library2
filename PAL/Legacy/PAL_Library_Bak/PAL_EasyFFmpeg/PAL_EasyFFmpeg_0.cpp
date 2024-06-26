/*
	PAL_EasyFFmpeg
	By:qianpinyi
	21.1.27~29
	
*/
#ifndef PAL_EASYFFMPEG_0_CPP
#define PAL_EASYFFMPEG_0_CPP 1

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <cmath>

#define __STDC_CONSTANT_MACROS

#ifndef INT64_C
	#define INT64_C(c) (c ## LL)
	#define UINT64_C(c) (c ## ULL)
#endif 

extern "C"
{
	#include <libavcodec/avcodec.h>
	#include <libavformat/avformat.h>
	#include <libavutil/time.h>
	#include <libavutil/opt.h>
	#include <libswscale/swscale.h>
	#include <libavcodec/avfft.h>
	#include <libswresample/swresample.h>
//#include "libavutil/avstring.h"
//#include "libavutil/eval.h"
//#include "libavutil/mathematics.h"
//#include "libavutil/pixdesc.h"
//#include "libavutil/imgutils.h"
//#include "libavutil/dict.h"
//#include "libavutil/parseutils.h"
//#include "libavutil/samplefmt.h"
//#include "libavutil/avassert.h"
//#include "libavutil/time.h"
//#include "libavdevice/avdevice.h"
//#include "libavutil/opt.h"
};

#include "../PAL_BasicFunctions/PAL_Debug.cpp"
#include "../PAL_BasicFunctions/PAL_BasicFunctions_0.cpp"
#include "../PAL_BasicFunctions/PAL_StringEX.cpp"

//Some of the code use ffplay directly, because it is hard for me to understand all.
namespace PAL_EasyFFmpeg 
{
	#define MAX_QUEUE_SIZE (15 * 1024 * 1024)
	#define MIN_FRAMES 25
	#define EXTERNAL_CLOCK_MIN_FRAMES 2
	#define EXTERNAL_CLOCK_MAX_FRAMES 10
	
	/* Minimum SDL audio buffer size, in samples. */
	#define SDL_AUDIO_MIN_BUFFER_SIZE 512
	/* Calculate actual buffer size keeping in mind not cause too frequent audio callbacks */
	#define SDL_AUDIO_MAX_CALLBACKS_PER_SEC 30
	
	/* Step size for volume control in dB */
	#define SDL_VOLUME_STEP (0.75)
	
	/* no AV sync correction is done if below the minimum AV sync threshold */
	#define AV_SYNC_THRESHOLD_MIN 0.04
	/* AV sync correction is done if above the maximum AV sync threshold */
	#define AV_SYNC_THRESHOLD_MAX 0.1
	/* If a frame duration is longer than this, it will not be duplicated to compensate AV sync */
	#define AV_SYNC_FRAMEDUP_THRESHOLD 0.1
	/* no AV correction is done if too big error */
	#define AV_NOSYNC_THRESHOLD 10.0
	
	/* maximum audio speed change to get correct sync */
	#define SAMPLE_CORRECTION_PERCENT_MAX 10
	
	/* external clock speed adjustment constants for realtime sources based on buffer fullness */
	#define EXTERNAL_CLOCK_SPEED_MIN  0.900
	#define EXTERNAL_CLOCK_SPEED_MAX  1.010
	#define EXTERNAL_CLOCK_SPEED_STEP 0.001
	
	/* we use about AUDIO_DIFF_AVG_NB A-V differences to make the average */
	#define AUDIO_DIFF_AVG_NB   20
	
	/* polls for possible required screen refresh at least this often, should be less than 1/fps */
	#define REFRESH_RATE 0.01
	
	/* NOTE: the size must be big enough to compensate the hardware audio buffersize size */
	/* TODO: We assume that a decoded and resampled frame fits into this buffer */
	#define SAMPLE_ARRAY_SIZE (8 * 65536)
	
//	#define CURSOR_HIDE_DELAY 1000000
	
	#define USE_ONEPASS_SUBTITLE_RENDER 1
	
	namespace _PAL_EasyFFmpeg
	{
		unsigned sws_flags = SWS_BICUBIC;
		
		struct MyAVPacketList
		{
			AVPacket pkt;
			MyAVPacketList *next;
			int serial;
		};
		
		struct PacketQueue
		{
			MyAVPacketList *first_pkt, *last_pkt;
			int nb_packets;
			int size;
			int64_t duration;
			int abort_request;
			int serial;
			SDL_mutex *mutex;
			SDL_cond *cond;
		};
		
	#define VIDEO_PICTURE_QUEUE_SIZE 3
	#define SUBPICTURE_QUEUE_SIZE 16
	#define SAMPLE_QUEUE_SIZE 9
	#define FRAME_QUEUE_SIZE FFMAX(SAMPLE_QUEUE_SIZE, FFMAX(VIDEO_PICTURE_QUEUE_SIZE, SUBPICTURE_QUEUE_SIZE))	
		
		struct AudioParams
		{
			int freq;
			int channels;
			int64_t channel_layout;
			enum AVSampleFormat fmt;
			int frame_size;
			int bytes_per_sec;
		};
		
		struct Clock
		{
			double pts;           /* clock base */
			double pts_drift;     /* clock base minus time at which we updated the clock */
			double last_updated;
			double speed;
			int serial;           /* clock is based on a packet with this serial */
			int paused;
			int *queue_serial;    /* pointer to the current packet queue serial, used for obsolete clock detection */
		};
		
		/* Common struct for handling all types of decoded data and allocated render buffers. */
		struct Frame
		{
			AVFrame *frame;
			AVSubtitle sub;
			int serial;
			double pts;           /* presentation timestamp for the frame */
			double duration;      /* estimated duration of the frame */
			int64_t pos;          /* byte position of the frame in the input file */
			int width;
			int height;
			int format;
			AVRational sar;
			int uploaded;
			int flip_v;
		};
		
		struct FrameQueue
		{
			Frame queue[FRAME_QUEUE_SIZE];
			int rindex;
			int windex;
			int size;
			int max_size;
			int keep_last;
			int rindex_shown;
			SDL_mutex *mutex;
			SDL_cond *cond;
			PacketQueue *pktq;
		};
		
		enum
		{
			AV_SYNC_AUDIO_MASTER, /* default choice */
			AV_SYNC_VIDEO_MASTER,
			AV_SYNC_EXTERNAL_CLOCK, /* synchronize to an external clock */
		};
		
		struct Decoder
		{
			AVPacket pkt;
			PacketQueue *queue;
			AVCodecContext *avctx;
			int pkt_serial;
			int finished;
			int packet_pending;
			SDL_cond *empty_queue_cond;
			int64_t start_pts;
			AVRational start_pts_tb;
			int64_t next_pts;
			AVRational next_pts_tb;
			SDL_Thread *decoder_tid;
		};
		
		enum ShowMode
		{
		    SHOW_MODE_NONE = -1, SHOW_MODE_VIDEO = 0, SHOW_MODE_WAVES, SHOW_MODE_RDFT, SHOW_MODE_NB
		};
		
		struct VideoState
		{
			SDL_Thread *read_tid;
			AVInputFormat *iformat;
			int abort_request;
			int force_refresh;
			int paused;
			int last_paused;
			int queue_attachments_req;
			int seek_req;
			int seek_flags;
			int64_t seek_pos;
			int64_t seek_rel;
			int read_pause_return;
			AVFormatContext *ic;
			int realtime;
			
			Clock audclk;
			Clock vidclk;
			Clock extclk;
			
			FrameQueue pictq;
			FrameQueue subpq;
			FrameQueue sampq;
			
			Decoder auddec;
			Decoder viddec;
			Decoder subdec;
			
			int audio_stream;
			
			int av_sync_type;
			
			double audio_clock;
			int audio_clock_serial;
			double audio_diff_cum; /* used for AV difference average computation */
			double audio_diff_avg_coef;
			double audio_diff_threshold;
			int audio_diff_avg_count;
			AVStream *audio_st;
			PacketQueue audioq;
			int audio_hw_buf_size;
			uint8_t *audio_buf;
			uint8_t *audio_buf1;
			unsigned int audio_buf_size; /* in bytes */
			unsigned int audio_buf1_size;
			int audio_buf_index; /* in bytes */
			int audio_write_buf_size;
			int audio_volume;
			int muted;
			AudioParams audio_src;
			AudioParams audio_tgt;
			SwrContext *swr_ctx;
			int frame_drops_early;
			int frame_drops_late;
			
			ShowMode show_mode;
			int16_t sample_array[SAMPLE_ARRAY_SIZE];
			int sample_array_index;
			int last_i_start;
			RDFTContext *rdft;
			int rdft_bits;
			FFTSample *rdft_data;
			int xpos;
			double last_vis_time;
			SDL_Texture *vis_texture;
			SDL_Texture *sub_texture;
			SDL_Texture *vid_texture;
			
			int subtitle_stream;
			AVStream *subtitle_st;
			PacketQueue subtitleq;
			
			double frame_timer;
			double frame_last_returned_time;
			double frame_last_filter_delay;
			int video_stream;
			AVStream *video_st;
			PacketQueue videoq;
			double max_frame_duration;      // maximum duration of a frame - above this, we consider the jump a timestamp discontinuity
			SwsContext *img_convert_ctx;
			SwsContext *sub_convert_ctx;
			int eof;
			
			char *filename;
			int width, height, xleft, ytop;
			int step;
			
			int last_video_stream, last_audio_stream, last_subtitle_stream;
			
			SDL_cond *continue_read_thread;
		};
		
		const struct TextureFormatEntry 
		{
			AVPixelFormat format;
			int texture_fmt;
		}sdl_texture_format_map[20]={
			{ AV_PIX_FMT_RGB8,           SDL_PIXELFORMAT_RGB332 },
			{ AV_PIX_FMT_RGB444,         SDL_PIXELFORMAT_RGB444 },
			{ AV_PIX_FMT_RGB555,         SDL_PIXELFORMAT_RGB555 },
			{ AV_PIX_FMT_BGR555,         SDL_PIXELFORMAT_BGR555 },
			{ AV_PIX_FMT_RGB565,         SDL_PIXELFORMAT_RGB565 },
			{ AV_PIX_FMT_BGR565,         SDL_PIXELFORMAT_BGR565 },
			{ AV_PIX_FMT_RGB24,          SDL_PIXELFORMAT_RGB24 },
			{ AV_PIX_FMT_BGR24,          SDL_PIXELFORMAT_BGR24 },
			{ AV_PIX_FMT_0RGB32,         SDL_PIXELFORMAT_RGB888 },
			{ AV_PIX_FMT_0BGR32,         SDL_PIXELFORMAT_BGR888 },
			{ AV_PIX_FMT_NE(RGB0, 0BGR), SDL_PIXELFORMAT_RGBX8888 },
			{ AV_PIX_FMT_NE(BGR0, 0RGB), SDL_PIXELFORMAT_BGRX8888 },
			{ AV_PIX_FMT_RGB32,          SDL_PIXELFORMAT_ARGB8888 },
			{ AV_PIX_FMT_RGB32_1,        SDL_PIXELFORMAT_RGBA8888 },
			{ AV_PIX_FMT_BGR32,          SDL_PIXELFORMAT_ABGR8888 },
			{ AV_PIX_FMT_BGR32_1,        SDL_PIXELFORMAT_BGRA8888 },
			{ AV_PIX_FMT_YUV420P,        SDL_PIXELFORMAT_IYUV },
			{ AV_PIX_FMT_YUYV422,        SDL_PIXELFORMAT_YUY2 },
			{ AV_PIX_FMT_UYVY422,        SDL_PIXELFORMAT_UYVY },
			{ AV_PIX_FMT_NONE,           SDL_PIXELFORMAT_UNKNOWN },
		};
		
		/* options specified by the user */
		AVInputFormat *file_iformat=NULL;
		int audio_disable=0;
		int video_disable=0;
		int subtitle_disable=0;
		const char* wanted_stream_spec[AVMEDIA_TYPE_NB]={0};
		int seek_by_bytes=-1;
		int display_disable=0;
		int startup_volume=100;
		int av_sync_type=AV_SYNC_AUDIO_MASTER;
		int64_t start_time=AV_NOPTS_VALUE;
		int64_t duration=AV_NOPTS_VALUE;
		int fast=0;
		int genpts=0;
		int lowres=0;
		int decoder_reorder_pts=-1;
		int autoexit=0;
		int loop=1;
		int framedrop=-1;
		int infinite_buffer=-1;
		ShowMode show_mode=SHOW_MODE_NONE;
		const char *audio_codec_name=NULL;
		const char *subtitle_codec_name=NULL;
		const char *video_codec_name=NULL;
		double rdftspeed=0.02;
		int autorotate=1;
		int find_stream_info=1;

		/* current context */
		int64_t audio_callback_time;
		AVPacket flush_pkt;
		SDL_AudioDeviceID audio_dev;
		VideoState *videostate=NULL;
		
		AVDictionary *codec_opts=NULL,*format_opts=NULL;//???

		Debug_Out PEF_DD;
		Uint32 _FF_QUIT_EVENT=0;
		Uint32 _FF_REACH_END=0;
		bool Inited=0;
		std::string srcFile;
		bool AudioOnly=0;
		int DisplaySize_W=0,
			DisplaySize_H=0;
		std::map <std::string,std::string> MediaMetaData;
		SDL_Renderer *renderer=NULL;
		int PrereadIsOK=0;
		
		int check_stream_specifier(AVFormatContext *s, AVStream *st, const char *spec)
		{
			int ret = avformat_match_stream_specifier(s, st, spec);
			if (ret < 0)
				PEF_DD[2]<<"Invalid stream specifier: "<<spec<<"!"<<endl;
			return ret;
		}
		
		AVDictionary *filter_codec_opts(AVDictionary *opts, enum AVCodecID codec_id,AVFormatContext *s, AVStream *st, AVCodec *codec)
		{
			AVDictionary    *ret = NULL;
			AVDictionaryEntry *t = NULL;
			int flags = s->oformat ? AV_OPT_FLAG_ENCODING_PARAM : AV_OPT_FLAG_DECODING_PARAM;
			char prefix = 0;
			const AVClass *cc = avcodec_get_class();

			if (!codec)
				codec = s->oformat ? avcodec_find_encoder(codec_id) : avcodec_find_decoder(codec_id);
				
			switch (st->codecpar->codec_type)
			{
				case AVMEDIA_TYPE_VIDEO:
					prefix = 'v';
					flags |= AV_OPT_FLAG_VIDEO_PARAM;
					break;
				case AVMEDIA_TYPE_AUDIO:
					prefix = 'a';
					flags |= AV_OPT_FLAG_AUDIO_PARAM;
					break;
				case AVMEDIA_TYPE_SUBTITLE:
					prefix = 's';
					flags |= AV_OPT_FLAG_SUBTITLE_PARAM;
					break;
			}

			while (t = av_dict_get(opts, "", t, AV_DICT_IGNORE_SUFFIX)) 
			{
				char *p = strchr(t->key, ':');
					
				/* check stream specification in opt name */
				if (p)
					switch (check_stream_specifier(s, st, p + 1)) 
					{
						case  1: *p = 0; break;
						case  0:         continue;
						default:         exit(2);//??
					}
					
				if (av_opt_find(&cc, t->key, NULL, flags, AV_OPT_SEARCH_FAKE_OBJ) || !codec || (codec->priv_class && av_opt_find(&codec->priv_class, t->key, NULL, flags, AV_OPT_SEARCH_FAKE_OBJ))) 
					av_dict_set(&ret, t->key, t->value, 0);
				else if (t->key[0] == prefix &&
					av_opt_find(&cc, t->key + 1, NULL, flags,AV_OPT_SEARCH_FAKE_OBJ))
				av_dict_set(&ret, t->key + 1, t->value, 0);
				
				if (p)
					*p = ':';
			}
			return ret;
		}
		
		AVDictionary **setup_find_stream_info_opts(AVFormatContext *s,AVDictionary *codec_opts)
	 	{
	 		int i;
	 		AVDictionary **opts;

	 		if (!s->nb_streams)
				return NULL;
			opts =(AVDictionary**) av_mallocz_array(s->nb_streams, sizeof(*opts));
			if (!opts)
			{
				PEF_DD[2]<<"Could not alloc memory for stream options."<<endl;
				return NULL;
			}
			for (i = 0; i < s->nb_streams; i++)
				opts[i] = filter_codec_opts(codec_opts, s->streams[i]->codecpar->codec_id,s, s->streams[i], NULL);
			return opts;
		}
		
		int packet_queue_put_private(PacketQueue *q, AVPacket *pkt)
		{
		    MyAVPacketList *pkt1;
		
		    if (q->abort_request)
		       return -1;
		
		    pkt1 =(MyAVPacketList*)av_malloc(sizeof(MyAVPacketList));
		    if (!pkt1)
		        return -1;
		    pkt1->pkt = *pkt;
		    pkt1->next = NULL;
		    if (pkt == &flush_pkt)
		        q->serial++;
		    pkt1->serial = q->serial;
		
		    if (!q->last_pkt)
		        q->first_pkt = pkt1;
		    else
		        q->last_pkt->next = pkt1;
		    q->last_pkt = pkt1;
		    q->nb_packets++;
		    q->size += pkt1->pkt.size + sizeof(*pkt1);
		    q->duration += pkt1->pkt.duration;
		    /* XXX: should duplicate packet data in DV case */
		    SDL_CondSignal(q->cond);
		    return 0;
		}
		
		int packet_queue_put(PacketQueue *q, AVPacket *pkt)
		{
		    int ret;
		
		    SDL_LockMutex(q->mutex);
		    ret = packet_queue_put_private(q, pkt);
		    SDL_UnlockMutex(q->mutex);
		
		    if (pkt != &flush_pkt && ret < 0)
		        av_packet_unref(pkt);
		
		    return ret;
		}
		
		int packet_queue_put_nullpacket(PacketQueue *q, int stream_index)
		{
		    AVPacket pkt1, *pkt = &pkt1;
		    av_init_packet(pkt);
		    pkt->data = NULL;
		    pkt->size = 0;
		    pkt->stream_index = stream_index;
		    return packet_queue_put(q, pkt);
		}
		
		/* packet queue handling */
		int packet_queue_init(PacketQueue *q)
		{
		    memset(q, 0, sizeof(PacketQueue));
		    q->mutex = SDL_CreateMutex();
		    if (!q->mutex) {
		    	PEF_DD[2]<<"SDL_CreateMutex Error: "<<SDL_GetError()<<endl;
		        return AVERROR(ENOMEM);
		    }
		    q->cond = SDL_CreateCond();
		    if (!q->cond) {
				PEF_DD[2]<<"SDL_CreateCond Error: "<<SDL_GetError()<<endl;
		        return AVERROR(ENOMEM);
		    }
		    q->abort_request = 1;
		    return 0;
		}
		
		void packet_queue_flush(PacketQueue *q)
		{
		    MyAVPacketList *pkt, *pkt1;
		
		    SDL_LockMutex(q->mutex);
		    for (pkt = q->first_pkt; pkt; pkt = pkt1) {
		        pkt1 = pkt->next;
		        av_packet_unref(&pkt->pkt);
		        av_freep(&pkt);
		    }
		    q->last_pkt = NULL;
		    q->first_pkt = NULL;
		    q->nb_packets = 0;
		    q->size = 0;
		    q->duration = 0;
		    SDL_UnlockMutex(q->mutex);
		}
		
		void packet_queue_destroy(PacketQueue *q)
		{
		    packet_queue_flush(q);
		    SDL_DestroyMutex(q->mutex);
		    SDL_DestroyCond(q->cond);
		}
		
		void packet_queue_abort(PacketQueue *q)
		{
		    SDL_LockMutex(q->mutex);
		    q->abort_request = 1;
		    SDL_CondSignal(q->cond);
		    SDL_UnlockMutex(q->mutex);
		}
		
		void packet_queue_start(PacketQueue *q)
		{
		    SDL_LockMutex(q->mutex);
		    q->abort_request = 0;
		    packet_queue_put_private(q, &flush_pkt);
		    SDL_UnlockMutex(q->mutex);
		}
		
		/* return < 0 if aborted, 0 if no packet and > 0 if packet.  */
		int packet_queue_get(PacketQueue *q, AVPacket *pkt, int block, int *serial)
		{
		    MyAVPacketList *pkt1;
		    int ret;
		
		    SDL_LockMutex(q->mutex);
		
		    for (;;) {
		        if (q->abort_request) {
		            ret = -1;
		            break;
		        }
		
		        pkt1 = q->first_pkt;
		        if (pkt1) {
		            q->first_pkt = pkt1->next;
		            if (!q->first_pkt)
		                q->last_pkt = NULL;
		            q->nb_packets--;
		            q->size -= pkt1->pkt.size + sizeof(*pkt1);
		            q->duration -= pkt1->pkt.duration;
		            *pkt = pkt1->pkt;
		            if (serial)
		                *serial = pkt1->serial;
		            av_free(pkt1);
		            ret = 1;
		            break;
		        } else if (!block) {
		            ret = 0;
		            break;
		        } else {
		            SDL_CondWait(q->cond, q->mutex);
		        }
		    }
		    SDL_UnlockMutex(q->mutex);
		    return ret;
		}
		
		void decoder_init(Decoder *d, AVCodecContext *avctx, PacketQueue *queue, SDL_cond *empty_queue_cond) {
		    memset(d, 0, sizeof(Decoder));
		    d->avctx = avctx;
		    d->queue = queue;
		    d->empty_queue_cond = empty_queue_cond;
		    d->start_pts = AV_NOPTS_VALUE;
		    d->pkt_serial = -1;
		}
		
		int decoder_decode_frame(Decoder *d, AVFrame *frame, AVSubtitle *sub) {
		    int ret = AVERROR(EAGAIN);
		
		    for (;;) {
		        AVPacket pkt;
		
		        if (d->queue->serial == d->pkt_serial) {
		            do {
		                if (d->queue->abort_request)
		                    return -1;
		
		                switch (d->avctx->codec_type) {
		                    case AVMEDIA_TYPE_VIDEO:
		                        ret = avcodec_receive_frame(d->avctx, frame);
		                        if (ret >= 0) {
		                            if (decoder_reorder_pts == -1) {
		                                frame->pts = frame->best_effort_timestamp;
		                            } else if (!decoder_reorder_pts) {
		                                frame->pts = frame->pkt_dts;
		                            }
		                        }
		                        break;
		                    case AVMEDIA_TYPE_AUDIO:
		                        ret = avcodec_receive_frame(d->avctx, frame);
		                        if (ret >= 0) {
		                            AVRational tb = (AVRational){1, frame->sample_rate};
		                            if (frame->pts != AV_NOPTS_VALUE)
		                                frame->pts = av_rescale_q(frame->pts, d->avctx->pkt_timebase, tb);
		                            else if (d->next_pts != AV_NOPTS_VALUE)
		                                frame->pts = av_rescale_q(d->next_pts, d->next_pts_tb, tb);
		                            if (frame->pts != AV_NOPTS_VALUE) {
		                                d->next_pts = frame->pts + frame->nb_samples;
		                                d->next_pts_tb = tb;
		                            }
		                        }
		                        break;
		                }
		                if (ret == AVERROR_EOF) {
		                    d->finished = d->pkt_serial;
		                    avcodec_flush_buffers(d->avctx);
		                    return 0;
		                }
		                if (ret >= 0)
		                    return 1;
		            } while (ret != AVERROR(EAGAIN));
		        }
		
		        do {
		            if (d->queue->nb_packets == 0)
		                SDL_CondSignal(d->empty_queue_cond);
		            if (d->packet_pending) {
		                av_packet_move_ref(&pkt, &d->pkt);
		                d->packet_pending = 0;
		            } else {
		                if (packet_queue_get(d->queue, &pkt, 1, &d->pkt_serial) < 0)
		                    return -1;
		            }
		        } while (d->queue->serial != d->pkt_serial);
		
		        if (pkt.data == flush_pkt.data) {
		            avcodec_flush_buffers(d->avctx);
		            d->finished = 0;
		            d->next_pts = d->start_pts;
		            d->next_pts_tb = d->start_pts_tb;
		        } else {
		            if (d->avctx->codec_type == AVMEDIA_TYPE_SUBTITLE) {
		                int got_frame = 0;
		                ret = avcodec_decode_subtitle2(d->avctx, sub, &got_frame, &pkt);
		                if (ret < 0) {
		                    ret = AVERROR(EAGAIN);
		                } else {
		                    if (got_frame && !pkt.data) {
		                       d->packet_pending = 1;
		                       av_packet_move_ref(&d->pkt, &pkt);
		                    }
		                    ret = got_frame ? 0 : (pkt.data ? AVERROR(EAGAIN) : AVERROR_EOF);
		                }
		            } else {
		                if (avcodec_send_packet(d->avctx, &pkt) == AVERROR(EAGAIN)) {
		                	PEF_DD[2]<<"Receive_frame and send_packet both returned EAGAIN, which is an API violation."<<endl;
		                    d->packet_pending = 1;
		                    av_packet_move_ref(&d->pkt, &pkt);
		                }
		            }
		            av_packet_unref(&pkt);
		        }
		    }
		}
		
		void decoder_destroy(Decoder *d) {
		    av_packet_unref(&d->pkt);
		    avcodec_free_context(&d->avctx);
		}
		
		void frame_queue_unref_item(Frame *vp)
		{
		    av_frame_unref(vp->frame);
		    avsubtitle_free(&vp->sub);
		}
		
		int frame_queue_init(FrameQueue *f, PacketQueue *pktq, int max_size, int keep_last)
		{
		    int i;
		    memset(f, 0, sizeof(FrameQueue));
		    if (!(f->mutex = SDL_CreateMutex())) {
		    	PEF_DD[2]<<"SDL_CreateMutex Error: "<<SDL_GetError()<<endl;
		        return AVERROR(ENOMEM);
		    }
		    if (!(f->cond = SDL_CreateCond())) {
		    	PEF_DD[2]<<"SDL_CreateCond Error: "<<SDL_GetError()<<endl;
		        return AVERROR(ENOMEM);
		    }
		    f->pktq = pktq;
		    f->max_size = FFMIN(max_size, FRAME_QUEUE_SIZE);
		    f->keep_last = !!keep_last;
		    for (i = 0; i < f->max_size; i++)
		        if (!(f->queue[i].frame = av_frame_alloc()))
		            return AVERROR(ENOMEM);
		    return 0;
		}
		
		void frame_queue_destory(FrameQueue *f)
		{
		    int i;
		    for (i = 0; i < f->max_size; i++) {
		        Frame *vp = &f->queue[i];
		        frame_queue_unref_item(vp);
		        av_frame_free(&vp->frame);
		    }
		    SDL_DestroyMutex(f->mutex);
		    SDL_DestroyCond(f->cond);
		}
		
		void frame_queue_signal(FrameQueue *f)
		{
		    SDL_LockMutex(f->mutex);
		    SDL_CondSignal(f->cond);
		    SDL_UnlockMutex(f->mutex);
		}
		
		Frame *frame_queue_peek(FrameQueue *f)
		{
		    return &f->queue[(f->rindex + f->rindex_shown) % f->max_size];
		}
		
		Frame *frame_queue_peek_next(FrameQueue *f)
		{
		    return &f->queue[(f->rindex + f->rindex_shown + 1) % f->max_size];
		}
		
		Frame *frame_queue_peek_last(FrameQueue *f)
		{
		    return &f->queue[f->rindex];
		}
		
		Frame *frame_queue_peek_writable(FrameQueue *f)
		{
		    /* wait until we have space to put a new frame */
		    SDL_LockMutex(f->mutex);
		    while (f->size >= f->max_size &&
		           !f->pktq->abort_request) {
		        SDL_CondWait(f->cond, f->mutex);
		    }
		    SDL_UnlockMutex(f->mutex);
		
		    if (f->pktq->abort_request)
		        return NULL;
		
		    return &f->queue[f->windex];
		}
		
		Frame *frame_queue_peek_readable(FrameQueue *f)
		{
		    /* wait until we have a readable a new frame */
		    SDL_LockMutex(f->mutex);
		    while (f->size - f->rindex_shown <= 0 &&
		           !f->pktq->abort_request) {
		        SDL_CondWait(f->cond, f->mutex);
		    }
		    SDL_UnlockMutex(f->mutex);
		
		    if (f->pktq->abort_request)
		        return NULL;
		
		    return &f->queue[(f->rindex + f->rindex_shown) % f->max_size];
		}
		
		void frame_queue_push(FrameQueue *f)
		{
		    if (++f->windex == f->max_size)
		        f->windex = 0;
		    SDL_LockMutex(f->mutex);
		    f->size++;
		    SDL_CondSignal(f->cond);
		    SDL_UnlockMutex(f->mutex);
		}
		
		void frame_queue_next(FrameQueue *f)
		{
		    if (f->keep_last && !f->rindex_shown) {
		        f->rindex_shown = 1;
		        return;
		    }
		    frame_queue_unref_item(&f->queue[f->rindex]);
		    if (++f->rindex == f->max_size)
		        f->rindex = 0;
		    SDL_LockMutex(f->mutex);
		    f->size--;
		    SDL_CondSignal(f->cond);
		    SDL_UnlockMutex(f->mutex);
		}
		
		/* return the number of undisplayed frames in the queue */
		int frame_queue_nb_remaining(FrameQueue *f)
		{
		    return f->size - f->rindex_shown;
		}
		/* return last shown position */
		int64_t frame_queue_last_pos(FrameQueue *f)
		{
		    Frame *fp = &f->queue[f->rindex];
		    if (f->rindex_shown && fp->serial == f->pktq->serial)
		        return fp->pos;
		    else
		        return -1;
		}
		
		void decoder_abort(Decoder *d, FrameQueue *fq)
		{
		    packet_queue_abort(d->queue);
		    frame_queue_signal(fq);
		    SDL_WaitThread(d->decoder_tid, NULL);
		    d->decoder_tid = NULL;
		    packet_queue_flush(d->queue);
		}
		
		int realloc_texture(SDL_Texture **texture, Uint32 new_format, int new_width, int new_height, SDL_BlendMode blendmode, int init_texture)
		{
		    Uint32 format;
		    int access, w, h;
		    if (!*texture || SDL_QueryTexture(*texture, &format, &access, &w, &h) < 0 || new_width != w || new_height != h || new_format != format) {
		        void *pixels;
		        int pitch;
		        if (*texture)
		            SDL_DestroyTexture(*texture);
		        if (!(*texture = SDL_CreateTexture(renderer, new_format, SDL_TEXTUREACCESS_STREAMING, new_width, new_height)))
		            return -1;
		        if (SDL_SetTextureBlendMode(*texture, blendmode) < 0)
		            return -1;
		        if (init_texture) {
		            if (SDL_LockTexture(*texture, NULL, &pixels, &pitch) < 0)
		                return -1;
		            memset(pixels, 0, pitch * new_height);
		            SDL_UnlockTexture(*texture);
		        }
		        PEF_DD[0]<<"Created "<<new_width<<"x"<<new_height<<" texture with "<<SDL_GetPixelFormatName(new_format)<<endl;
		    }
		    return 0;
		}
		
		void calculate_display_rect(SDL_Rect *rect,
                                   int scr_xleft, int scr_ytop, int scr_width, int scr_height,
                                   int pic_width, int pic_height, AVRational pic_sar)
		{
		    float aspect_ratio;
		    int width, height, x, y;
		
		    if (pic_sar.num == 0)
		        aspect_ratio = 0;
		    else
		        aspect_ratio = av_q2d(pic_sar);
		
		    if (aspect_ratio <= 0.0)
		        aspect_ratio = 1.0;
		    aspect_ratio *= (float)pic_width / (float)pic_height;
		
		    /* XXX: we suppose the screen has a 1.0 pixel ratio */
		    height = scr_height;
		    width = lrint(height * aspect_ratio) & ~1;
		    if (width > scr_width) {
		        width = scr_width;
		        height = lrint(width / aspect_ratio) & ~1;
		    }
		    x = (scr_width - width) / 2;
		    y = (scr_height - height) / 2;
		    rect->x = scr_xleft + x;
		    rect->y = scr_ytop  + y;
		    rect->w = FFMAX(width,  1);
		    rect->h = FFMAX(height, 1);
		}
		
		void get_sdl_pix_fmt_and_blendmode(int format, Uint32 *sdl_pix_fmt, SDL_BlendMode *sdl_blendmode)
		{
		    int i;
		    *sdl_blendmode = SDL_BLENDMODE_NONE;
		    *sdl_pix_fmt = SDL_PIXELFORMAT_UNKNOWN;
		    if (format == AV_PIX_FMT_RGB32   ||
		        format == AV_PIX_FMT_RGB32_1 ||
		        format == AV_PIX_FMT_BGR32   ||
		        format == AV_PIX_FMT_BGR32_1)
		        *sdl_blendmode = SDL_BLENDMODE_BLEND;
		    for (i = 0; i < FF_ARRAY_ELEMS(sdl_texture_format_map) - 1; i++) {
		        if (format == sdl_texture_format_map[i].format) {
		            *sdl_pix_fmt = sdl_texture_format_map[i].texture_fmt;
		            return;
		        }
		    }
		}
		
		int upload_texture(SDL_Texture **tex, AVFrame *frame, struct SwsContext **img_convert_ctx)
		{
		    int ret = 0;
		    Uint32 sdl_pix_fmt;
		    SDL_BlendMode sdl_blendmode;
		    get_sdl_pix_fmt_and_blendmode(frame->format, &sdl_pix_fmt, &sdl_blendmode);
		    if (realloc_texture(tex, sdl_pix_fmt == SDL_PIXELFORMAT_UNKNOWN ? SDL_PIXELFORMAT_ARGB8888 : sdl_pix_fmt, frame->width, frame->height, sdl_blendmode, 0) < 0)
		        return -1;
		    switch (sdl_pix_fmt) {
		        case SDL_PIXELFORMAT_UNKNOWN:
		            /* This should only happen if we are not using avfilter... */
		            *img_convert_ctx = sws_getCachedContext(*img_convert_ctx,
		                frame->width, frame->height,(AVPixelFormat) frame->format, frame->width, frame->height,
		                AV_PIX_FMT_BGRA, sws_flags, NULL, NULL, NULL);
		            if (*img_convert_ctx != NULL) {
		                uint8_t *pixels[4];
		                int pitch[4];
		                if (!SDL_LockTexture(*tex, NULL, (void **)pixels, pitch)) {
		                    sws_scale(*img_convert_ctx, (const uint8_t * const *)frame->data, frame->linesize,
		                              0, frame->height, pixels, pitch);
		                    SDL_UnlockTexture(*tex);
		                }
		            } else {
		            	PEF_DD[2]<<"Cannot initialize the conversion context"<<endl;
		                ret = -1;
		            }
		            break;
		        case SDL_PIXELFORMAT_IYUV:
		            if (frame->linesize[0] > 0 && frame->linesize[1] > 0 && frame->linesize[2] > 0) {
		                ret = SDL_UpdateYUVTexture(*tex, NULL, frame->data[0], frame->linesize[0],
		                                                       frame->data[1], frame->linesize[1],
		                                                       frame->data[2], frame->linesize[2]);
		            } else if (frame->linesize[0] < 0 && frame->linesize[1] < 0 && frame->linesize[2] < 0) {
		                ret = SDL_UpdateYUVTexture(*tex, NULL, frame->data[0] + frame->linesize[0] * (frame->height                    - 1), -frame->linesize[0],
		                                                       frame->data[1] + frame->linesize[1] * (AV_CEIL_RSHIFT(frame->height, 1) - 1), -frame->linesize[1],
		                                                       frame->data[2] + frame->linesize[2] * (AV_CEIL_RSHIFT(frame->height, 1) - 1), -frame->linesize[2]);
		            } else {
		            	PEF_DD[2]<<"Mixed negative and positive linesizes are not supported."<<endl;
		                return -1;
		            }
		            break;
		        default:
		            if (frame->linesize[0] < 0) {
		                ret = SDL_UpdateTexture(*tex, NULL, frame->data[0] + frame->linesize[0] * (frame->height - 1), -frame->linesize[0]);
		            } else {
		                ret = SDL_UpdateTexture(*tex, NULL, frame->data[0], frame->linesize[0]);
		            }
		            break;
		    }
		    return ret;
		}
		
		void set_sdl_yuv_conversion_mode(AVFrame *frame)
		{
		#if SDL_VERSION_ATLEAST(2,0,8)
		    SDL_YUV_CONVERSION_MODE mode = SDL_YUV_CONVERSION_AUTOMATIC;
		    if (frame && (frame->format == AV_PIX_FMT_YUV420P || frame->format == AV_PIX_FMT_YUYV422 || frame->format == AV_PIX_FMT_UYVY422)) {
		        if (frame->color_range == AVCOL_RANGE_JPEG)
		            mode = SDL_YUV_CONVERSION_JPEG;
		        else if (frame->colorspace == AVCOL_SPC_BT709)
		            mode = SDL_YUV_CONVERSION_BT709;
		        else if (frame->colorspace == AVCOL_SPC_BT470BG || frame->colorspace == AVCOL_SPC_SMPTE170M || frame->colorspace == AVCOL_SPC_SMPTE240M)
		            mode = SDL_YUV_CONVERSION_BT601;
		    }
		    SDL_SetYUVConversionMode(mode);
		#endif
		}
		
		int video_image_display(VideoState *is)//return 1 or 2 for display
		{
		    Frame *vp;
		    Frame *sp = NULL;
		    SDL_Rect rect;
		
		    vp = frame_queue_peek_last(&is->pictq);
		    if (is->subtitle_st) {
		        if (frame_queue_nb_remaining(&is->subpq) > 0) {
		            sp = frame_queue_peek(&is->subpq);
		
		            if (vp->pts >= sp->pts + ((float) sp->sub.start_display_time / 1000)) {
		                if (!sp->uploaded) {
		                    uint8_t* pixels[4];
		                    int pitch[4];
		                    int i;
		                    if (!sp->width || !sp->height) {
		                        sp->width = vp->width;
		                        sp->height = vp->height;
		                    }
		                    if (realloc_texture(&is->sub_texture, SDL_PIXELFORMAT_ARGB8888, sp->width, sp->height, SDL_BLENDMODE_BLEND, 1) < 0)
		                        return -1;
		
		                    for (i = 0; i < sp->sub.num_rects; i++) {
		                        AVSubtitleRect *sub_rect = sp->sub.rects[i];
		
		                        sub_rect->x = av_clip(sub_rect->x, 0, sp->width );
		                        sub_rect->y = av_clip(sub_rect->y, 0, sp->height);
		                        sub_rect->w = av_clip(sub_rect->w, 0, sp->width  - sub_rect->x);
		                        sub_rect->h = av_clip(sub_rect->h, 0, sp->height - sub_rect->y);
		
		                        is->sub_convert_ctx = sws_getCachedContext(is->sub_convert_ctx,
		                            sub_rect->w, sub_rect->h, AV_PIX_FMT_PAL8,
		                            sub_rect->w, sub_rect->h, AV_PIX_FMT_BGRA,
		                            0, NULL, NULL, NULL);
		                        if (!is->sub_convert_ctx) {
		                        	PEF_DD[2]<<"Cannot initialize the conversion context"<<endl;
		                            return -1;
		                        }
		                        if (!SDL_LockTexture(is->sub_texture, (SDL_Rect *)sub_rect, (void **)pixels, pitch)) {
		                            sws_scale(is->sub_convert_ctx, (const uint8_t * const *)sub_rect->data, sub_rect->linesize,
		                                      0, sub_rect->h, pixels, pitch);
		                            SDL_UnlockTexture(is->sub_texture);
		                        }
		                    }
		                    sp->uploaded = 1;
		                }
		            } else
		                sp = NULL;
		        }
		    }
		
		    calculate_display_rect(&rect, is->xleft, is->ytop, is->width, is->height, vp->width, vp->height, vp->sar);
		
		    if (!vp->uploaded) {
		        if (upload_texture(&is->vid_texture, vp->frame, &is->img_convert_ctx) < 0)
		            return -1;
		        vp->uploaded = 1;
		        vp->flip_v = vp->frame->linesize[0] < 0;
		    }
		
//		    set_sdl_yuv_conversion_mode(vp->frame);
//		    SDL_RenderCopyEx(renderer, is->vid_texture, NULL, &rect, 0, NULL, vp->flip_v ? SDL_FLIP_VERTICAL : 0);
//		    set_sdl_yuv_conversion_mode(NULL);
		    if (sp) {
//		#if USE_ONEPASS_SUBTITLE_RENDER
//		        SDL_RenderCopy(renderer, is->sub_texture, NULL, &rect);
//		#else
//		        int i;
//		        double xratio = (double)rect.w / (double)sp->width;
//		        double yratio = (double)rect.h / (double)sp->height;
//		        for (i = 0; i < sp->sub.num_rects; i++) {
//		            SDL_Rect *sub_rect = (SDL_Rect*)sp->sub.rects[i];
//		            SDL_Rect target = {.x = rect.x + sub_rect->x * xratio,
//		                               .y = rect.y + sub_rect->y * yratio,
//		                               .w = sub_rect->w * xratio,
//		                               .h = sub_rect->h * yratio};
//		            SDL_RenderCopy(renderer, is->sub_texture, sub_rect, &target);
//		        }
//		#endif
				return 2;
		    }
		    else return 1;
		}
		
		inline int compute_mod(int a, int b)
		{
		    return a < 0 ? a%b + b : a%b;
		}
		
		int video_audio_display(VideoState *s)//return 3 for display
		{
			PEF_DD[2]<<"video_audio_display cannot use yet!"<<endl;
			return -1;
//		    int i, i_start, x, y1, y, ys, delay, n, nb_display_channels;
//		    int ch, channels, h, h2;
//		    int64_t time_diff;
//		    int rdft_bits, nb_freq;
//		
//		    for (rdft_bits = 1; (1 << rdft_bits) < 2 * s->height; rdft_bits++)
//		        ;
//		    nb_freq = 1 << (rdft_bits - 1);
//		
//		    /* compute display index : center on currently output samples */
//		    channels = s->audio_tgt.channels;
//		    nb_display_channels = channels;
//		    if (!s->paused) {
//		        int data_used= s->show_mode == SHOW_MODE_WAVES ? s->width : (2*nb_freq);
//		        n = 2 * channels;
//		        delay = s->audio_write_buf_size;
//		        delay /= n;
//		
//		        /* to be more precise, we take into account the time spent since
//		           the last buffer computation */
//		        if (audio_callback_time) {
//		            time_diff = av_gettime_relative() - audio_callback_time;
//		            delay -= (time_diff * s->audio_tgt.freq) / 1000000;
//		        }
//		
//		        delay += 2 * data_used;
//		        if (delay < data_used)
//		            delay = data_used;
//		
//		        i_start= x = compute_mod(s->sample_array_index - delay * channels, SAMPLE_ARRAY_SIZE);
//		        if (s->show_mode == SHOW_MODE_WAVES) {
//		            h = INT_MIN;
//		            for (i = 0; i < 1000; i += channels) {
//		                int idx = (SAMPLE_ARRAY_SIZE + x - i) % SAMPLE_ARRAY_SIZE;
//		                int a = s->sample_array[idx];
//		                int b = s->sample_array[(idx + 4 * channels) % SAMPLE_ARRAY_SIZE];
//		                int c = s->sample_array[(idx + 5 * channels) % SAMPLE_ARRAY_SIZE];
//		                int d = s->sample_array[(idx + 9 * channels) % SAMPLE_ARRAY_SIZE];
//		                int score = a - d;
//		                if (h < score && (b ^ c) < 0) {
//		                    h = score;
//		                    i_start = idx;
//		                }
//		            }
//		        }
//		
//		        s->last_i_start = i_start;
//		    } else {
//		        i_start = s->last_i_start;
//		    }
//		
//		    if (s->show_mode == SHOW_MODE_WAVES) {
//		        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
//		
//		        /* total height for one channel */
//		        h = s->height / nb_display_channels;
//		        /* graph height / 2 */
//		        h2 = (h * 9) / 20;
//		        for (ch = 0; ch < nb_display_channels; ch++) {
//		            i = i_start + ch;
//		            y1 = s->ytop + ch * h + (h / 2); /* position of center line */
//		            for (x = 0; x < s->width; x++) {
//		                y = (s->sample_array[i] * h2) >> 15;
//		                if (y < 0) {
//		                    y = -y;
//		                    ys = y1 - y;
//		                } else {
//		                    ys = y1;
//		                }
//		                fill_rectangle(s->xleft + x, ys, 1, y);
//		                i += channels;
//		                if (i >= SAMPLE_ARRAY_SIZE)
//		                    i -= SAMPLE_ARRAY_SIZE;
//		            }
//		        }
//		
//		        SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
//		
//		        for (ch = 1; ch < nb_display_channels; ch++) {
//		            y = s->ytop + ch * h;
//		            fill_rectangle(s->xleft, y, s->width, 1);
//		        }
//		    } else {
//		        if (realloc_texture(&s->vis_texture, SDL_PIXELFORMAT_ARGB8888, s->width, s->height, SDL_BLENDMODE_NONE, 1) < 0)
//		            return;
//		
//		        nb_display_channels= FFMIN(nb_display_channels, 2);
//		        if (rdft_bits != s->rdft_bits) {
//		            av_rdft_end(s->rdft);
//		            av_free(s->rdft_data);
//		            s->rdft = av_rdft_init(rdft_bits, DFT_R2C);
//		            s->rdft_bits = rdft_bits;
//		            s->rdft_data = av_malloc_array(nb_freq, 4 *sizeof(*s->rdft_data));
//		        }
//		        if (!s->rdft || !s->rdft_data){
//		            av_log(NULL, AV_LOG_ERROR, "Failed to allocate buffers for RDFT, switching to waves display\n");
//		            s->show_mode = SHOW_MODE_WAVES;
//		        } else {
//		            FFTSample *data[2];
//		            SDL_Rect rect = {.x = s->xpos, .y = 0, .w = 1, .h = s->height};
//		            uint32_t *pixels;
//		            int pitch;
//		            for (ch = 0; ch < nb_display_channels; ch++) {
//		                data[ch] = s->rdft_data + 2 * nb_freq * ch;
//		                i = i_start + ch;
//		                for (x = 0; x < 2 * nb_freq; x++) {
//		                    double w = (x-nb_freq) * (1.0 / nb_freq);
//		                    data[ch][x] = s->sample_array[i] * (1.0 - w * w);
//		                    i += channels;
//		                    if (i >= SAMPLE_ARRAY_SIZE)
//		                        i -= SAMPLE_ARRAY_SIZE;
//		                }
//		                av_rdft_calc(s->rdft, data[ch]);
//		            }
//		            /* Least efficient way to do this, we should of course
//		             * directly access it but it is more than fast enough. */
//		            if (!SDL_LockTexture(s->vis_texture, &rect, (void **)&pixels, &pitch)) {
//		                pitch >>= 2;
//		                pixels += pitch * s->height;
//		                for (y = 0; y < s->height; y++) {
//		                    double w = 1 / sqrt(nb_freq);
//		                    int a = sqrt(w * sqrt(data[0][2 * y + 0] * data[0][2 * y + 0] + data[0][2 * y + 1] * data[0][2 * y + 1]));
//		                    int b = (nb_display_channels == 2 ) ? sqrt(w * hypot(data[1][2 * y + 0], data[1][2 * y + 1]))
//		                                                        : a;
//		                    a = FFMIN(a, 255);
//		                    b = FFMIN(b, 255);
//		                    pixels -= pitch;
//		                    *pixels = (a << 16) + (b << 8) + ((a+b) >> 1);
//		                }
//		                SDL_UnlockTexture(s->vis_texture);
//		            }
//		            SDL_RenderCopy(renderer, s->vis_texture, NULL, NULL);
//		        }
//		        if (!s->paused)
//		            s->xpos++;
//		        if (s->xpos >= s->width)
//		            s->xpos= s->xleft;
//		    }
		}
		
		void stream_component_close(VideoState *is, int stream_index)
		{
		    AVFormatContext *ic = is->ic;
		    AVCodecParameters *codecpar;
		
		    if (stream_index < 0 || stream_index >= ic->nb_streams)
		        return;
		    codecpar = ic->streams[stream_index]->codecpar;
		
		    switch (codecpar->codec_type) {
		    case AVMEDIA_TYPE_AUDIO:
		        decoder_abort(&is->auddec, &is->sampq);
		        SDL_CloseAudioDevice(audio_dev);
		        decoder_destroy(&is->auddec);
		        swr_free(&is->swr_ctx);
		        av_freep(&is->audio_buf1);
		        is->audio_buf1_size = 0;
		        is->audio_buf = NULL;
		
		        if (is->rdft) {
		            av_rdft_end(is->rdft);
		            av_freep(&is->rdft_data);
		            is->rdft = NULL;
		            is->rdft_bits = 0;
		        }
		        break;
		    case AVMEDIA_TYPE_VIDEO:
		        decoder_abort(&is->viddec, &is->pictq);
		        decoder_destroy(&is->viddec);
		        break;
		    case AVMEDIA_TYPE_SUBTITLE:
		        decoder_abort(&is->subdec, &is->subpq);
		        decoder_destroy(&is->subdec);
		        break;
		    default:
		        break;
		    }
		
		    ic->streams[stream_index]->discard = AVDISCARD_ALL;
		    switch (codecpar->codec_type) {
		    case AVMEDIA_TYPE_AUDIO:
		        is->audio_st = NULL;
		        is->audio_stream = -1;
		        break;
		    case AVMEDIA_TYPE_VIDEO:
		        is->video_st = NULL;
		        is->video_stream = -1;
		        break;
		    case AVMEDIA_TYPE_SUBTITLE:
		        is->subtitle_st = NULL;
		        is->subtitle_stream = -1;
		        break;
		    default:
		        break;
		    }
		}
		
		void stream_close(VideoState *is)
		{
		    /* XXX: use a special url_shutdown call to abort parse cleanly */
		    is->abort_request = 1;
		    SDL_WaitThread(is->read_tid,NULL);
		
		    /* close each stream */
		    if (is->audio_stream >= 0)
		        stream_component_close(is, is->audio_stream);
		    if (is->video_stream >= 0)
		        stream_component_close(is, is->video_stream);
		    if (is->subtitle_stream >= 0)
		        stream_component_close(is, is->subtitle_stream);
		
		    avformat_close_input(&is->ic);
		
		    packet_queue_destroy(&is->videoq);
		    packet_queue_destroy(&is->audioq);
		    packet_queue_destroy(&is->subtitleq);
		
		    /* free all pictures */
		    frame_queue_destory(&is->pictq);
		    frame_queue_destory(&is->sampq);
		    frame_queue_destory(&is->subpq);
		    SDL_DestroyCond(is->continue_read_thread);
		    sws_freeContext(is->img_convert_ctx);
		    sws_freeContext(is->sub_convert_ctx);
		    av_free(is->filename);
		    if (is->vis_texture)
		        SDL_DestroyTexture(is->vis_texture);
		    if (is->vid_texture)
		        SDL_DestroyTexture(is->vid_texture);
		    if (is->sub_texture)
		        SDL_DestroyTexture(is->sub_texture);
		    av_free(is);
		}
		
		void do_exit(VideoState *is)
		{
		    if (is)
				stream_close(is);
		}
		
		void set_default_window_size(int width, int height, AVRational sar)
		{
//		    SDL_Rect rect;
//		    calculate_display_rect(&rect, 0, 0, INT_MAX, height, width, height, sar);
//		    default_width  = rect.w;
//		    default_height = rect.h;
		}
		
		int video_open(VideoState *is)
		{
		    int w,h;
			
			w=DisplaySize_W;
			h=DisplaySize_H;
		
		    is->width  = w;
		    is->height = h;
		
		    return 0;
		}
		
		/* display the current picture, if any */
		int video_display(VideoState *is)
		{
		    if (!is->width)
		        video_open(is);
		
		    if (is->audio_st && is->show_mode != SHOW_MODE_VIDEO)
		        return video_audio_display(is);
		    else if (is->video_st)
		        return video_image_display(is);
			return 0;
		}
		
		double get_clock(Clock *c)
		{
		    if (*c->queue_serial != c->serial)
		        return NAN;
		    if (c->paused) {
		        return c->pts;
		    } else {
		        double time = av_gettime_relative() / 1000000.0;
		        return c->pts_drift + time - (time - c->last_updated) * (1.0 - c->speed);
		    }
		}
		
		void set_clock_at(Clock *c, double pts, int serial, double time)
		{
		    c->pts = pts;
		    c->last_updated = time;
		    c->pts_drift = c->pts - time;
		    c->serial = serial;
		}
		
		void set_clock(Clock *c, double pts, int serial)
		{
		    double time = av_gettime_relative() / 1000000.0;
		    set_clock_at(c, pts, serial, time);
		}
		
		void set_clock_speed(Clock *c, double speed)
		{
		    set_clock(c, get_clock(c), c->serial);
		    c->speed = speed;
		}
		
		void init_clock(Clock *c,int *queue_serial)
		{
		    c->speed = 1.0;
		    c->paused = 0;
		    c->queue_serial = queue_serial;
		    set_clock(c, NAN, -1);
		}
		
		void sync_clock_to_slave(Clock *c, Clock *slave)
		{
		    double clock = get_clock(c);
		    double slave_clock = get_clock(slave);
		    if (!std::isnan(slave_clock) && (std::isnan(clock) || fabs(clock - slave_clock) > AV_NOSYNC_THRESHOLD))
		        set_clock(c, slave_clock, slave->serial);
		}
		
		int get_master_sync_type(VideoState *is) {
		    if (is->av_sync_type == AV_SYNC_VIDEO_MASTER) {
		        if (is->video_st)
		            return AV_SYNC_VIDEO_MASTER;
		        else
		            return AV_SYNC_AUDIO_MASTER;
		    } else if (is->av_sync_type == AV_SYNC_AUDIO_MASTER) {
		        if (is->audio_st)
		            return AV_SYNC_AUDIO_MASTER;
		        else
		            return AV_SYNC_EXTERNAL_CLOCK;
		    } else {
		        return AV_SYNC_EXTERNAL_CLOCK;
		    }
		}
		
		/* get the current master clock value */
		double get_master_clock(VideoState *is)
		{
		    double val;
		
		    switch (get_master_sync_type(is)) {
		        case AV_SYNC_VIDEO_MASTER:
		            val = get_clock(&is->vidclk);
		            break;
		        case AV_SYNC_AUDIO_MASTER:
		            val = get_clock(&is->audclk);
		            break;
		        default:
		            val = get_clock(&is->extclk);
		            break;
		    }
		    return val;
		}
		
		void check_external_clock_speed(VideoState *is) {
		   if (is->video_stream >= 0 && is->videoq.nb_packets <= EXTERNAL_CLOCK_MIN_FRAMES ||
		       is->audio_stream >= 0 && is->audioq.nb_packets <= EXTERNAL_CLOCK_MIN_FRAMES) {
		       set_clock_speed(&is->extclk, FFMAX(EXTERNAL_CLOCK_SPEED_MIN, is->extclk.speed - EXTERNAL_CLOCK_SPEED_STEP));
		   } else if ((is->video_stream < 0 || is->videoq.nb_packets > EXTERNAL_CLOCK_MAX_FRAMES) &&
		              (is->audio_stream < 0 || is->audioq.nb_packets > EXTERNAL_CLOCK_MAX_FRAMES)) {
		       set_clock_speed(&is->extclk, FFMIN(EXTERNAL_CLOCK_SPEED_MAX, is->extclk.speed + EXTERNAL_CLOCK_SPEED_STEP));
		   } else {
		       double speed = is->extclk.speed;
		       if (speed != 1.0)
		           set_clock_speed(&is->extclk, speed + EXTERNAL_CLOCK_SPEED_STEP * (1.0 - speed) / fabs(1.0 - speed));
		   }
		}
		
		/* seek in the stream */
		void stream_seek(VideoState *is, int64_t pos, int64_t rel, int seek_by_bytes)
		{
		    if (!is->seek_req) {
		        is->seek_pos = pos;
		        is->seek_rel = rel;
		        is->seek_flags &= ~AVSEEK_FLAG_BYTE;
		        if (seek_by_bytes)
		            is->seek_flags |= AVSEEK_FLAG_BYTE;
		        is->seek_req = 1;
		        SDL_CondSignal(is->continue_read_thread);
		    }
		}
		
		/* pause or resume the video */
		void stream_toggle_pause(VideoState *is)
		{
		    if (is->paused) {
		        is->frame_timer += av_gettime_relative() / 1000000.0 - is->vidclk.last_updated;
		        if (is->read_pause_return != AVERROR(ENOSYS)) {
		            is->vidclk.paused = 0;
		        }
		        set_clock(&is->vidclk, get_clock(&is->vidclk), is->vidclk.serial);
		    }
		    set_clock(&is->extclk, get_clock(&is->extclk), is->extclk.serial);
		    is->paused = is->audclk.paused = is->vidclk.paused = is->extclk.paused = !is->paused;
		}
		
		void toggle_pause(VideoState *is)
		{
		    stream_toggle_pause(is);
		    is->step = 0;
		}
		
		void toggle_mute(VideoState *is)
		{
		    is->muted = !is->muted;
		}
		
		void update_volume(VideoState *is, int sign, double step)
		{
		    double volume_level = is->audio_volume ? (20 * log(is->audio_volume / (double)SDL_MIX_MAXVOLUME) / log(10)) : -1000.0;
		    int new_volume = lrint(SDL_MIX_MAXVOLUME * pow(10.0, (volume_level + sign * step) / 20.0));
		    is->audio_volume = av_clip(is->audio_volume == new_volume ? (is->audio_volume + sign) : new_volume, 0, SDL_MIX_MAXVOLUME);
		}
		
		void step_to_next_frame(VideoState *is)
		{
		    /* if the stream is paused unpause it, then step */
		    if (is->paused)
		        stream_toggle_pause(is);
		    is->step = 1;
		}
		
		double compute_target_delay(double delay, VideoState *is)
		{
		    double sync_threshold, diff = 0;
		
		    /* update delay to follow master synchronisation source */
		    if (get_master_sync_type(is) != AV_SYNC_VIDEO_MASTER) {
		        /* if video is slave, we try to correct big delays by
		           duplicating or deleting a frame */
		        diff = get_clock(&is->vidclk) - get_master_clock(is);
		
		        /* skip or repeat frame. We take into account the
		           delay to compute the threshold. I still don't know
		           if it is the best guess */
		        sync_threshold = FFMAX(AV_SYNC_THRESHOLD_MIN, FFMIN(AV_SYNC_THRESHOLD_MAX, delay));
		        if (!std::isnan(diff) && std::fabs(diff) < is->max_frame_duration) {
		            if (diff <= -sync_threshold)
		                delay = FFMAX(0, delay + diff);
		            else if (diff >= sync_threshold && delay > AV_SYNC_FRAMEDUP_THRESHOLD)
		                delay = delay + diff;
		            else if (diff >= sync_threshold)
		                delay = 2 * delay;
		        }
		    }
		
//			PEF_DD[3]<<"video: delay="<<delay<<" A-V="<<-diff<<endl;
			
		    return delay;
		}
		
		double vp_duration(VideoState *is, Frame *vp, Frame *nextvp) {
		    if (vp->serial == nextvp->serial) {
		        double duration = nextvp->pts - vp->pts;
		        if (std::isnan(duration) || duration <= 0 || duration > is->max_frame_duration)
		            return vp->duration;
		        else
		            return duration;
		    } else {
		        return 0.0;
		    }
		}
		
		void update_video_pts(VideoState *is, double pts, int64_t pos, int serial) {
		    /* update current video pts */
		    set_clock(&is->vidclk, pts, serial);
		    sync_clock_to_slave(&is->extclk, &is->vidclk);
		}
		
		/* called to display each frame */
		int video_refresh(void *opaque, double *remaining_time)
		{
		    VideoState *is =(VideoState*) opaque;
		    double time;
		    int texToShow=0;
		
		    Frame *sp, *sp2;
		
		    if (!is->paused && get_master_sync_type(is) == AV_SYNC_EXTERNAL_CLOCK && is->realtime)
		        check_external_clock_speed(is);
		
		    if (!display_disable && is->show_mode != SHOW_MODE_VIDEO && is->audio_st) {
		        time = av_gettime_relative() / 1000000.0;
		        if (is->force_refresh || is->last_vis_time + rdftspeed < time) {
		            texToShow=video_display(is);
		            is->last_vis_time = time;
		        }
		        *remaining_time = FFMIN(*remaining_time, is->last_vis_time + rdftspeed - time);
		    }
		
		    if (is->video_st) {
		retry:
		        if (frame_queue_nb_remaining(&is->pictq) == 0) {
		            // nothing to do, no picture to display in the queue
		        } else {
		            double last_duration, duration, delay;
		            Frame *vp, *lastvp;
		
		            /* dequeue the picture */
		            lastvp = frame_queue_peek_last(&is->pictq);
		            vp = frame_queue_peek(&is->pictq);
		
		            if (vp->serial != is->videoq.serial) {
		                frame_queue_next(&is->pictq);
		                goto retry;
		            }
		
		            if (lastvp->serial != vp->serial)
		                is->frame_timer = av_gettime_relative() / 1000000.0;
		
		            if (is->paused)
		                goto display;
		
		            /* compute nominal last_duration */
		            last_duration = vp_duration(is, lastvp, vp);
		            delay = compute_target_delay(last_duration, is);
		
		            time= av_gettime_relative()/1000000.0;
		            if (time < is->frame_timer + delay) {
		                *remaining_time = FFMIN(is->frame_timer + delay - time, *remaining_time);
		                goto display;
		            }
		
		            is->frame_timer += delay;
		            if (delay > 0 && time - is->frame_timer > AV_SYNC_THRESHOLD_MAX)
		                is->frame_timer = time;
		
		            SDL_LockMutex(is->pictq.mutex);
		            if (!std::isnan(vp->pts))
		                update_video_pts(is, vp->pts, vp->pos, vp->serial);
		            SDL_UnlockMutex(is->pictq.mutex);
		
		            if (frame_queue_nb_remaining(&is->pictq) > 1) {
		                Frame *nextvp = frame_queue_peek_next(&is->pictq);
		                duration = vp_duration(is, vp, nextvp);
		                if(!is->step && (framedrop>0 || (framedrop && get_master_sync_type(is) != AV_SYNC_VIDEO_MASTER)) && time > is->frame_timer + duration){
		                    is->frame_drops_late++;
		                    frame_queue_next(&is->pictq);
		                    goto retry;
		                }
		            }
		
		            if (is->subtitle_st) {
		                    while (frame_queue_nb_remaining(&is->subpq) > 0) {
		                        sp = frame_queue_peek(&is->subpq);
		
		                        if (frame_queue_nb_remaining(&is->subpq) > 1)
		                            sp2 = frame_queue_peek_next(&is->subpq);
		                        else
		                            sp2 = NULL;
		
		                        if (sp->serial != is->subtitleq.serial
		                                || (is->vidclk.pts > (sp->pts + ((float) sp->sub.end_display_time / 1000)))
		                                || (sp2 && is->vidclk.pts > (sp2->pts + ((float) sp2->sub.start_display_time / 1000))))
		                        {
		                            if (sp->uploaded) {
		                                int i;
		                                for (i = 0; i < sp->sub.num_rects; i++) {
		                                    AVSubtitleRect *sub_rect = sp->sub.rects[i];
		                                    uint8_t *pixels;
		                                    int pitch, j;
		
		                                    if (!SDL_LockTexture(is->sub_texture, (SDL_Rect *)sub_rect, (void **)&pixels, &pitch)) {
		                                        for (j = 0; j < sub_rect->h; j++, pixels += pitch)
		                                            memset(pixels, 0, sub_rect->w << 2);
		                                        SDL_UnlockTexture(is->sub_texture);
		                                    }
		                                }
		                            }
		                            frame_queue_next(&is->subpq);
		                        } else {
		                            break;
		                        }
		                    }
		            }
		
		            frame_queue_next(&is->pictq);
		            is->force_refresh = 1;
		
		            if (is->step && !is->paused)
		                stream_toggle_pause(is);
		        }
		display:
		        /* display picture */
		        if (!display_disable && is->force_refresh && is->show_mode == SHOW_MODE_VIDEO && is->pictq.rindex_shown)
		        	texToShow=video_display(is);
		    }
		    is->force_refresh = 0;
//		    if (show_status) {
//		        static int64_t last_time;
//		        int64_t cur_time;
//		        int aqsize, vqsize, sqsize;
//		        double av_diff;
//		
//		        cur_time = av_gettime_relative();
//		        if (!last_time || (cur_time - last_time) >= 30000) {
//		            aqsize = 0;
//		            vqsize = 0;
//		            sqsize = 0;
//		            if (is->audio_st)
//		                aqsize = is->audioq.size;
//		            if (is->video_st)
//		                vqsize = is->videoq.size;
//		            if (is->subtitle_st)
//		                sqsize = is->subtitleq.size;
//		            av_diff = 0;
//		            if (is->audio_st && is->video_st)
//		                av_diff = get_clock(&is->audclk) - get_clock(&is->vidclk);
//		            else if (is->video_st)
//		                av_diff = get_master_clock(is) - get_clock(&is->vidclk);
//		            else if (is->audio_st)
//		                av_diff = get_master_clock(is) - get_clock(&is->audclk);
//		            av_log(NULL, AV_LOG_INFO,
//		                   "%7.2f %s:%7.3f fd=%4d aq=%5dKB vq=%5dKB sq=%5dB f=%"PRId64"/%"PRId64"   \r",
//		                   get_master_clock(is),
//		                   (is->audio_st && is->video_st) ? "A-V" : (is->video_st ? "M-V" : (is->audio_st ? "M-A" : "   ")),
//		                   av_diff,
//		                   is->frame_drops_early + is->frame_drops_late,
//		                   aqsize / 1024,
//		                   vqsize / 1024,
//		                   sqsize,
//		                   is->video_st ? is->viddec.avctx->pts_correction_num_faulty_dts : 0,
//		                   is->video_st ? is->viddec.avctx->pts_correction_num_faulty_pts : 0);
//		            fflush(stdout);
//		            last_time = cur_time;
//		        }
//		    }
		    return texToShow;
		}
		
		int queue_picture(VideoState *is, AVFrame *src_frame, double pts, double duration, int64_t pos, int serial)
		{
		    Frame *vp;
		
		#if defined(DEBUG_SYNC)
		    printf("frame_type=%c pts=%0.3f\n",
		           av_get_picture_type_char(src_frame->pict_type), pts);
		#endif
		
		    if (!(vp = frame_queue_peek_writable(&is->pictq)))
		        return -1;
		
		    vp->sar = src_frame->sample_aspect_ratio;
		    vp->uploaded = 0;
		
		    vp->width = src_frame->width;
		    vp->height = src_frame->height;
		    vp->format = src_frame->format;
		
		    vp->pts = pts;
		    vp->duration = duration;
		    vp->pos = pos;
		    vp->serial = serial;
		
		    set_default_window_size(vp->width, vp->height, vp->sar);
		
		    av_frame_move_ref(vp->frame, src_frame);
		    frame_queue_push(&is->pictq);
		    return 0;
		}
		
		int get_video_frame(VideoState *is, AVFrame *frame)
		{
		    int got_picture;
		
		    if ((got_picture = decoder_decode_frame(&is->viddec, frame, NULL)) < 0)
		        return -1;
		
		    if (got_picture) {
		        double dpts = NAN;
		
		        if (frame->pts != AV_NOPTS_VALUE)
		            dpts = av_q2d(is->video_st->time_base) * frame->pts;
		
		        frame->sample_aspect_ratio = av_guess_sample_aspect_ratio(is->ic, is->video_st, frame);
		
		        if (framedrop>0 || (framedrop && get_master_sync_type(is) != AV_SYNC_VIDEO_MASTER)) {
		            if (frame->pts != AV_NOPTS_VALUE) {
		                double diff = dpts - get_master_clock(is);
		                if (!std::isnan(diff) && std::fabs(diff) < AV_NOSYNC_THRESHOLD &&
		                    diff - is->frame_last_filter_delay < 0 &&
		                    is->viddec.pkt_serial == is->vidclk.serial &&
		                    is->videoq.nb_packets) {
		                    is->frame_drops_early++;
		                    av_frame_unref(frame);
		                    got_picture = 0;
		                }
		            }
		        }
		    }
		
		    return got_picture;
		}
		
		int audio_thread(void *arg)
		{
		    VideoState *is =(VideoState*) arg;
		    AVFrame *frame = av_frame_alloc();
		    Frame *af;
		    int got_frame = 0;
		    AVRational tb;
		    int ret = 0;
		
		    if (!frame)
		        return AVERROR(ENOMEM);
		
		    do {
		        if ((got_frame = decoder_decode_frame(&is->auddec, frame, NULL)) < 0)
		            goto the_end;
		
		        if (got_frame) {
		                tb = (AVRational){1, frame->sample_rate};
		
		                if (!(af = frame_queue_peek_writable(&is->sampq)))
		                    goto the_end;
		
		                af->pts = (frame->pts == AV_NOPTS_VALUE) ? NAN : frame->pts * av_q2d(tb);
		                af->pos = frame->pkt_pos;
		                af->serial = is->auddec.pkt_serial;
		                af->duration = av_q2d((AVRational){frame->nb_samples, frame->sample_rate});
		
		                av_frame_move_ref(af->frame, frame);
		                frame_queue_push(&is->sampq);
		        }
		    } while (ret >= 0 || ret == AVERROR(EAGAIN) || ret == AVERROR_EOF);
		 the_end:
		    av_frame_free(&frame);
		    return ret;
		}
		
		int decoder_start(Decoder *d, int (*fn)(void *), void *arg)
		{
		    packet_queue_start(d->queue);
		    d->decoder_tid = SDL_CreateThread(fn, "decoder", arg);
		    if (!d->decoder_tid) {
		    	PEF_DD[2]<<"SDL_CreateThread Error: "<<SDL_GetError()<<endl;
		        return AVERROR(ENOMEM);
		    }
		    return 0;
		}
		
		int video_thread(void *arg)
		{
		    VideoState *is =(VideoState*) arg;
		    AVFrame *frame = av_frame_alloc();
		    double pts;
		    double duration;
		    int ret;
		    AVRational tb = is->video_st->time_base;
		    AVRational frame_rate = av_guess_frame_rate(is->ic, is->video_st, NULL);
		
		    if (!frame)
		        return AVERROR(ENOMEM);
		
		    for (;;) {
		        ret = get_video_frame(is, frame);
		        if (ret < 0)
		            goto the_end;
		        if (!ret)
		            continue;
		
	            duration = (frame_rate.num && frame_rate.den ? av_q2d((AVRational){frame_rate.den, frame_rate.num}) : 0);
	            pts = (frame->pts == AV_NOPTS_VALUE) ? NAN : frame->pts * av_q2d(tb);
	            ret = queue_picture(is, frame, pts, duration, frame->pkt_pos, is->viddec.pkt_serial);
	            av_frame_unref(frame);
		
		        if (ret < 0)
		            goto the_end;
		    }
		 the_end:
		    av_frame_free(&frame);
		    return 0;
		}
		
		int subtitle_thread(void *arg)
		{
		    VideoState *is =(VideoState*) arg;
		    Frame *sp;
		    int got_subtitle;
		    double pts;
		
		    for (;;) {
		        if (!(sp = frame_queue_peek_writable(&is->subpq)))
		            return 0;
		
		        if ((got_subtitle = decoder_decode_frame(&is->subdec, NULL, &sp->sub)) < 0)
		            break;
		
		        pts = 0;
		
		        if (got_subtitle && sp->sub.format == 0) {
		            if (sp->sub.pts != AV_NOPTS_VALUE)
		                pts = sp->sub.pts / (double)AV_TIME_BASE;
		            sp->pts = pts;
		            sp->serial = is->subdec.pkt_serial;
		            sp->width = is->subdec.avctx->width;
		            sp->height = is->subdec.avctx->height;
		            sp->uploaded = 0;
		
		            /* now we can update the picture count */
		            frame_queue_push(&is->subpq);
		        } else if (got_subtitle) {
		            avsubtitle_free(&sp->sub);
		        }
		    }
		    return 0;
		}
		
		/* copy samples for viewing in editor window */
		void update_sample_display(VideoState *is, short *samples, int samples_size)
		{
		    int size, len;
		
		    size = samples_size / sizeof(short);
		    while (size > 0) {
		        len = SAMPLE_ARRAY_SIZE - is->sample_array_index;
		        if (len > size)
		            len = size;
		        memcpy(is->sample_array + is->sample_array_index, samples, len * sizeof(short));
		        samples += len;
		        is->sample_array_index += len;
		        if (is->sample_array_index >= SAMPLE_ARRAY_SIZE)
		            is->sample_array_index = 0;
		        size -= len;
		    }
		}
		
		/* return the wanted number of samples to get better sync if sync_type is video
		 * or external master clock */
		int synchronize_audio(VideoState *is, int nb_samples)
		{
		    int wanted_nb_samples = nb_samples;
		
		    /* if not master, then we try to remove or add samples to correct the clock */
		    if (get_master_sync_type(is) != AV_SYNC_AUDIO_MASTER) {
		        double diff, avg_diff;
		        int min_nb_samples, max_nb_samples;
		
		        diff = get_clock(&is->audclk) - get_master_clock(is);
		
		        if (!std::isnan(diff) && std::fabs(diff) < AV_NOSYNC_THRESHOLD) {
		            is->audio_diff_cum = diff + is->audio_diff_avg_coef * is->audio_diff_cum;
		            if (is->audio_diff_avg_count < AUDIO_DIFF_AVG_NB) {
		                /* not enough measures to have a correct estimate */
		                is->audio_diff_avg_count++;
		            } else {
		                /* estimate the A-V difference */
		                avg_diff = is->audio_diff_cum * (1.0 - is->audio_diff_avg_coef);
		
		                if (fabs(avg_diff) >= is->audio_diff_threshold) {
		                    wanted_nb_samples = nb_samples + (int)(diff * is->audio_src.freq);
		                    min_nb_samples = ((nb_samples * (100 - SAMPLE_CORRECTION_PERCENT_MAX) / 100));
		                    max_nb_samples = ((nb_samples * (100 + SAMPLE_CORRECTION_PERCENT_MAX) / 100));
		                    wanted_nb_samples = av_clip(wanted_nb_samples, min_nb_samples, max_nb_samples);
		                }
		                PEF_DD[3]<<"diff="<<diff<<" adiff="<<avg_diff<<" sample_diff="<<wanted_nb_samples - nb_samples<<" apts="<<is->audio_clock<<" "<<is->audio_diff_threshold<<endl;
		            }
		        } else {
		            /* too big difference : may be initial PTS errors, so
		               reset A-V filter */
		            is->audio_diff_avg_count = 0;
		            is->audio_diff_cum       = 0;
		        }
		    }
		
		    return wanted_nb_samples;
		}
		
		/**
		 * Decode one audio frame and return its uncompressed size.
		 *
		 * The processed audio frame is decoded, converted if required, and
		 * stored in is->audio_buf, with size in bytes given by the return
		 * value.
		 */
		int audio_decode_frame(VideoState *is)
		{
		    int data_size, resampled_data_size;
		    int64_t dec_channel_layout;
		    av_unused double audio_clock0;
		    int wanted_nb_samples;
		    Frame *af;
		
		    if (is->paused)
		        return -1;
		
		    do {
		#if defined(_WIN32)
		        while (frame_queue_nb_remaining(&is->sampq) == 0) {
		            if ((av_gettime_relative() - audio_callback_time) > 1000000LL * is->audio_hw_buf_size / is->audio_tgt.bytes_per_sec / 2)
		                return -1;
		            av_usleep (1000);
		        }
		#endif
		        if (!(af = frame_queue_peek_readable(&is->sampq)))
		            return -1;
		        frame_queue_next(&is->sampq);
		    } while (af->serial != is->audioq.serial);
		
		    data_size = av_samples_get_buffer_size(NULL, af->frame->channels,
		                                           af->frame->nb_samples,
		                                           (AVSampleFormat)af->frame->format, 1);
		
		    dec_channel_layout =
		        (af->frame->channel_layout && af->frame->channels == av_get_channel_layout_nb_channels(af->frame->channel_layout)) ?
		        af->frame->channel_layout : av_get_default_channel_layout(af->frame->channels);
		    wanted_nb_samples = synchronize_audio(is, af->frame->nb_samples);
		
		    if (af->frame->format        != is->audio_src.fmt            ||
		        dec_channel_layout       != is->audio_src.channel_layout ||
		        af->frame->sample_rate   != is->audio_src.freq           ||
		        (wanted_nb_samples       != af->frame->nb_samples && !is->swr_ctx)) {
		        swr_free(&is->swr_ctx);
		        is->swr_ctx = swr_alloc_set_opts(NULL,
		                                         is->audio_tgt.channel_layout, is->audio_tgt.fmt, is->audio_tgt.freq,
		                                         dec_channel_layout,           (AVSampleFormat)af->frame->format, af->frame->sample_rate,
		                                         0, NULL);
		        if (!is->swr_ctx || swr_init(is->swr_ctx) < 0) {
		            PEF_DD[2]<<"Cannot create sample rate converter for conversion of "<<af->frame->sample_rate<<" Hz "
								<<av_get_sample_fmt_name((AVSampleFormat)af->frame->format)<<" "<<af->frame->channels<<" channels to "
								<<is->audio_tgt.freq<<" Hz "<<av_get_sample_fmt_name(is->audio_tgt.fmt)<<" "<<is->audio_tgt.channels<<" channels!"<<endl;
		            swr_free(&is->swr_ctx);
		            return -1;
		        }
		        is->audio_src.channel_layout = dec_channel_layout;
		        is->audio_src.channels       = af->frame->channels;
		        is->audio_src.freq = af->frame->sample_rate;
		        is->audio_src.fmt =(AVSampleFormat) af->frame->format;
		    }
		
		    if (is->swr_ctx) {
		        const uint8_t **in = (const uint8_t **)af->frame->extended_data;
		        uint8_t **out = &is->audio_buf1;
		        int out_count = (int64_t)wanted_nb_samples * is->audio_tgt.freq / af->frame->sample_rate + 256;
		        int out_size  = av_samples_get_buffer_size(NULL, is->audio_tgt.channels, out_count, is->audio_tgt.fmt, 0);
		        int len2;
		        if (out_size < 0) {
		        	PEF_DD[2]<<"av_samples_get_buffer_size() failed"<<endl;
		            return -1;
		        }
		        if (wanted_nb_samples != af->frame->nb_samples) {
		            if (swr_set_compensation(is->swr_ctx, (wanted_nb_samples - af->frame->nb_samples) * is->audio_tgt.freq / af->frame->sample_rate,
		                                        wanted_nb_samples * is->audio_tgt.freq / af->frame->sample_rate) < 0) {
		                PEF_DD[2]<<"swr_set_compensation() failed"<<endl;
		                return -1;
		            }
		        }
		        av_fast_malloc(&is->audio_buf1, &is->audio_buf1_size, out_size);
		        if (!is->audio_buf1)
		            return AVERROR(ENOMEM);
		        len2 = swr_convert(is->swr_ctx, out, out_count, in, af->frame->nb_samples);
		        if (len2 < 0) {
		        	PEF_DD[2]<<"swr_convert() failed"<<endl;
		            return -1;
		        }
		        if (len2 == out_count) {
		        	PEF_DD[1]<<"audio buffer is probably too small"<<endl;
		            if (swr_init(is->swr_ctx) < 0)
		                swr_free(&is->swr_ctx);
		        }
		        is->audio_buf = is->audio_buf1;
		        resampled_data_size = len2 * is->audio_tgt.channels * av_get_bytes_per_sample(is->audio_tgt.fmt);
		    } else {
		        is->audio_buf = af->frame->data[0];
		        resampled_data_size = data_size;
		    }
		
		    audio_clock0 = is->audio_clock;
		    /* update the audio clock with the pts */
		    if (!std::isnan(af->pts))
		        is->audio_clock = af->pts + (double) af->frame->nb_samples / af->frame->sample_rate;
		    else
		        is->audio_clock = NAN;
		    is->audio_clock_serial = af->serial;
//		#ifdef DEBUG
//		    {
//		        static double last_clock;
//		        printf("audio: delay=%0.3f clock=%0.3f clock0=%0.3f\n",
//		               is->audio_clock - last_clock,
//		               is->audio_clock, audio_clock0);
//		        last_clock = is->audio_clock;
//		    }
//		#endif
		    return resampled_data_size;
		}
		
		/* prepare a new audio buffer */
		void sdl_audio_callback(void *opaque, Uint8 *stream, int len)
		{
		    VideoState *is =(VideoState*) opaque;
		    int audio_size, len1;
		
		    audio_callback_time = av_gettime_relative();
		
		    while (len > 0) {
		        if (is->audio_buf_index >= is->audio_buf_size) {
		           audio_size = audio_decode_frame(is);
		           if (audio_size < 0) {
		                /* if error, just output silence */
		               is->audio_buf = NULL;
		               is->audio_buf_size = SDL_AUDIO_MIN_BUFFER_SIZE / is->audio_tgt.frame_size * is->audio_tgt.frame_size;
		           } else {
		               if (is->show_mode != SHOW_MODE_VIDEO)
		                   update_sample_display(is, (int16_t *)is->audio_buf, audio_size);
		               is->audio_buf_size = audio_size;
		           }
		           is->audio_buf_index = 0;
		        }
		        len1 = is->audio_buf_size - is->audio_buf_index;
		        if (len1 > len)
		            len1 = len;
		        if (!is->muted && is->audio_buf && is->audio_volume == SDL_MIX_MAXVOLUME)
		            memcpy(stream, (uint8_t *)is->audio_buf + is->audio_buf_index, len1);
		        else {
		            memset(stream, 0, len1);
		            if (!is->muted && is->audio_buf)
		                SDL_MixAudioFormat(stream, (uint8_t *)is->audio_buf + is->audio_buf_index, AUDIO_S16SYS, len1, is->audio_volume);
		        }
		        len -= len1;
		        stream += len1;
		        is->audio_buf_index += len1;
		    }
		    is->audio_write_buf_size = is->audio_buf_size - is->audio_buf_index;
		    /* Let's assume the audio driver that is used by SDL has two periods. */
		    if (!std::isnan(is->audio_clock)) {
		        set_clock_at(&is->audclk, is->audio_clock - (double)(2 * is->audio_hw_buf_size + is->audio_write_buf_size) / is->audio_tgt.bytes_per_sec, is->audio_clock_serial, audio_callback_time / 1000000.0);
		        sync_clock_to_slave(&is->extclk, &is->audclk);
		    }
		}
		
		int audio_open(void *opaque, int64_t wanted_channel_layout, int wanted_nb_channels, int wanted_sample_rate, struct AudioParams *audio_hw_params)
		{
		    SDL_AudioSpec wanted_spec, spec;
		    const char *env;
		    static const int next_nb_channels[] = {0, 0, 1, 6, 2, 6, 4, 6};
		    static const int next_sample_rates[] = {0, 44100, 48000, 96000, 192000};
		    int next_sample_rate_idx = FF_ARRAY_ELEMS(next_sample_rates) - 1;
		
		    env = SDL_getenv("SDL_AUDIO_CHANNELS");
		    if (env) {
		        wanted_nb_channels = atoi(env);
		        wanted_channel_layout = av_get_default_channel_layout(wanted_nb_channels);
		    }
		    if (!wanted_channel_layout || wanted_nb_channels != av_get_channel_layout_nb_channels(wanted_channel_layout)) {
		        wanted_channel_layout = av_get_default_channel_layout(wanted_nb_channels);
		        wanted_channel_layout &= ~AV_CH_LAYOUT_STEREO_DOWNMIX;
		    }
		    wanted_nb_channels = av_get_channel_layout_nb_channels(wanted_channel_layout);
		    wanted_spec.channels = wanted_nb_channels;
		    wanted_spec.freq = wanted_sample_rate;
		    if (wanted_spec.freq <= 0 || wanted_spec.channels <= 0) {
		    	PEF_DD[2]<<"Invalid sample rate or channel count!"<<endl;
		        return -1;
		    }
		    while (next_sample_rate_idx && next_sample_rates[next_sample_rate_idx] >= wanted_spec.freq)
		        next_sample_rate_idx--;
		    wanted_spec.format = AUDIO_S16SYS;
		    wanted_spec.silence = 0;
		    wanted_spec.samples = FFMAX(SDL_AUDIO_MIN_BUFFER_SIZE, 2 << av_log2(wanted_spec.freq / SDL_AUDIO_MAX_CALLBACKS_PER_SEC));
		    wanted_spec.callback = sdl_audio_callback;
		    wanted_spec.userdata = opaque;
		    while (!(audio_dev = SDL_OpenAudioDevice(NULL, 0, &wanted_spec, &spec, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE | SDL_AUDIO_ALLOW_CHANNELS_CHANGE))) {
		    	PEF_DD[1]<<"SDL_OpenAudio ("<<wanted_spec.channels<<" channels, "<<wanted_spec.freq<<" Hz): "<<SDL_GetError()<<endl;
		        wanted_spec.channels = next_nb_channels[FFMIN(7, wanted_spec.channels)];
		        if (!wanted_spec.channels) {
		            wanted_spec.freq = next_sample_rates[next_sample_rate_idx--];
		            wanted_spec.channels = wanted_nb_channels;
		            if (!wanted_spec.freq) {
		            	PEF_DD[2]<<"No more combinations to try, audio open failed"<<endl;
		                return -1;
		            }
		        }
		        wanted_channel_layout = av_get_default_channel_layout(wanted_spec.channels);
		    }
		    if (spec.format != AUDIO_S16SYS) {
		    	PEF_DD[2]<<"SDL advised audio format "<<spec.format<<" is not supported!"<<endl;
		        return -1;
		    }
		    if (spec.channels != wanted_spec.channels) {
		        wanted_channel_layout = av_get_default_channel_layout(spec.channels);
		        if (!wanted_channel_layout) {
		        	PEF_DD[2]<<"SDL advised channel count "<<spec.channels<<" is not supported!"<<endl;
		            return -1;
		        }
		    }
		
		    audio_hw_params->fmt = AV_SAMPLE_FMT_S16;
		    audio_hw_params->freq = spec.freq;
		    audio_hw_params->channel_layout = wanted_channel_layout;
		    audio_hw_params->channels =  spec.channels;
		    audio_hw_params->frame_size = av_samples_get_buffer_size(NULL, audio_hw_params->channels, 1, audio_hw_params->fmt, 1);
		    audio_hw_params->bytes_per_sec = av_samples_get_buffer_size(NULL, audio_hw_params->channels, audio_hw_params->freq, audio_hw_params->fmt, 1);
		    if (audio_hw_params->bytes_per_sec <= 0 || audio_hw_params->frame_size <= 0) {
		        PEF_DD[2]<<"av_samples_get_buffer_size failed"<<endl;
		        return -1;
		    }
		    return spec.size;
		}
		
		/* open a given stream. Return 0 if OK */
		int stream_component_open(VideoState *is, int stream_index)
		{
		    AVFormatContext *ic = is->ic;
		    AVCodecContext *avctx;
		    AVCodec *codec;
		    const char *forced_codec_name = NULL;
		    AVDictionary *opts = NULL;
		    AVDictionaryEntry *t = NULL;
		    int sample_rate, nb_channels;
		    int64_t channel_layout;
		    int ret = 0;
		    int stream_lowres = lowres;
		
		    if (stream_index < 0 || stream_index >= ic->nb_streams)
		        return -1;
		
		    avctx = avcodec_alloc_context3(NULL);
		    if (!avctx)
		        return AVERROR(ENOMEM);
		
		    ret = avcodec_parameters_to_context(avctx, ic->streams[stream_index]->codecpar);
		    if (ret < 0)
		        goto fail;
		    avctx->pkt_timebase = ic->streams[stream_index]->time_base;
		
		    codec = avcodec_find_decoder(avctx->codec_id);
		
		    switch(avctx->codec_type){
		        case AVMEDIA_TYPE_AUDIO   : is->last_audio_stream    = stream_index; forced_codec_name =    audio_codec_name; break;
		        case AVMEDIA_TYPE_SUBTITLE: is->last_subtitle_stream = stream_index; forced_codec_name = subtitle_codec_name; break;
		        case AVMEDIA_TYPE_VIDEO   : is->last_video_stream    = stream_index; forced_codec_name =    video_codec_name; break;
		    }
		    if (forced_codec_name)
		        codec = avcodec_find_decoder_by_name(forced_codec_name);
		    if (!codec) {
		        if (forced_codec_name)
					PEF_DD[1]<<"No codec could be found with name "<<forced_codec_name<<endl;
		        else
					PEF_DD[1]<<"No decoder could be found for codec "<<avcodec_get_name(avctx->codec_id)<<endl;
		        ret = AVERROR(EINVAL);
		        goto fail;
		    }
		
		    avctx->codec_id = codec->id;
		    if (stream_lowres > codec->max_lowres) {
		    	PEF_DD[1]<<"The maximum value for lowres supported by the decoder is "<<codec->max_lowres<<endl;
		        stream_lowres = codec->max_lowres;
		    }
		    avctx->lowres = stream_lowres;
		
		    if (fast)
		        avctx->flags2 |= AV_CODEC_FLAG2_FAST;
		
		    opts = filter_codec_opts(codec_opts, avctx->codec_id, ic, ic->streams[stream_index], codec);
		    if (!av_dict_get(opts, "threads", NULL, 0))
		        av_dict_set(&opts, "threads", "auto", 0);
		    if (stream_lowres)
		        av_dict_set_int(&opts, "lowres", stream_lowres, 0);
		    if (avctx->codec_type == AVMEDIA_TYPE_VIDEO || avctx->codec_type == AVMEDIA_TYPE_AUDIO)
		        av_dict_set(&opts, "refcounted_frames", "1", 0);
		    if ((ret = avcodec_open2(avctx, codec, &opts)) < 0) {
		        goto fail;
		    }
		    if ((t = av_dict_get(opts, "", NULL, AV_DICT_IGNORE_SUFFIX))) {
		    	PEF_DD[2]<<"Option "<<t->key<<" not found!"<<endl;
		        ret =  AVERROR_OPTION_NOT_FOUND;
		        goto fail;
		    }
		
		    is->eof = 0;
		    ic->streams[stream_index]->discard = AVDISCARD_DEFAULT;
		    switch (avctx->codec_type) {
		    case AVMEDIA_TYPE_AUDIO:
		        sample_rate    = avctx->sample_rate;
		        nb_channels    = avctx->channels;
		        channel_layout = avctx->channel_layout;

		        /* prepare audio output */
		        if ((ret = audio_open(is, channel_layout, nb_channels, sample_rate, &is->audio_tgt)) < 0)
		            goto fail;
		        is->audio_hw_buf_size = ret;
		        is->audio_src = is->audio_tgt;
		        is->audio_buf_size  = 0;
		        is->audio_buf_index = 0;
		
		        /* init averaging filter */
		        is->audio_diff_avg_coef  = exp(log(0.01) / AUDIO_DIFF_AVG_NB);
		        is->audio_diff_avg_count = 0;
		        /* since we do not have a precise anough audio FIFO fullness,
		           we correct audio sync only if larger than this threshold */
		        is->audio_diff_threshold = (double)(is->audio_hw_buf_size) / is->audio_tgt.bytes_per_sec;
		
		        is->audio_stream = stream_index;
		        is->audio_st = ic->streams[stream_index];
		
		        decoder_init(&is->auddec, avctx, &is->audioq, is->continue_read_thread);
		        if ((is->ic->iformat->flags & (AVFMT_NOBINSEARCH | AVFMT_NOGENSEARCH | AVFMT_NO_BYTE_SEEK)) && !is->ic->iformat->read_seek) {
		            is->auddec.start_pts = is->audio_st->start_time;
		            is->auddec.start_pts_tb = is->audio_st->time_base;
		        }
		        if ((ret = decoder_start(&is->auddec, audio_thread, is)) < 0)
		            goto out;
		        SDL_PauseAudioDevice(audio_dev, 0);
		        break;
		    case AVMEDIA_TYPE_VIDEO:
		        is->video_stream = stream_index;
		        is->video_st = ic->streams[stream_index];
		
		        decoder_init(&is->viddec, avctx, &is->videoq, is->continue_read_thread);
		        if ((ret = decoder_start(&is->viddec, video_thread, is)) < 0)
		            goto out;
		        is->queue_attachments_req = 1;
		        break;
		    case AVMEDIA_TYPE_SUBTITLE:
		        is->subtitle_stream = stream_index;
		        is->subtitle_st = ic->streams[stream_index];
		
		        decoder_init(&is->subdec, avctx, &is->subtitleq, is->continue_read_thread);
		        if ((ret = decoder_start(&is->subdec, subtitle_thread, is)) < 0)
		            goto out;
		        break;
		    default:
		        break;
		    }
		    goto out;
		
		fail:
		    avcodec_free_context(&avctx);
		out:
		    av_dict_free(&opts);
		
		    return ret;
		}
		
		int decode_interrupt_cb(void *ctx)
		{
		    VideoState *is =(VideoState*) ctx;
		    return is->abort_request;
		}
		
		int stream_has_enough_packets(AVStream *st, int stream_id, PacketQueue *queue) {
		    return stream_id < 0 ||
		           queue->abort_request ||
		           (st->disposition & AV_DISPOSITION_ATTACHED_PIC) ||
		           queue->nb_packets > MIN_FRAMES && (!queue->duration || av_q2d(st->time_base) * queue->duration > 1.0);
		}

		
		int is_realtime(AVFormatContext *s)
		{
		    if(   !strcmp(s->iformat->name, "rtp")
		       || !strcmp(s->iformat->name, "rtsp")
		       || !strcmp(s->iformat->name, "sdp")
		    )
		        return 1;
		
		    if(s->pb && (   !strncmp(s->url, "rtp:", 4)
		                 || !strncmp(s->url, "udp:", 4)
		                )
		    )
		        return 1;
		    return 0;
		}
		
		/* this thread gets the stream from the disk or the network */
		int read_thread(void *arg)
		{
		    VideoState *is =(VideoState*) arg;
		    AVFormatContext *ic = NULL;
		    int err, i, ret;
		    int st_index[AVMEDIA_TYPE_NB];
		    AVPacket pkt1, *pkt = &pkt1;
		    int64_t stream_start_time;
		    int pkt_in_play_range = 0;
		    AVDictionaryEntry *t;
		    SDL_mutex *wait_mutex = SDL_CreateMutex();
		    int scan_all_pmts_set = 0;
		    int64_t pkt_ts;
		    
			bool reach_end_flag=0;
		
		    if (!wait_mutex) {
		    	PEF_DD[2]<<"SDL_CreateMutex Error: "<<SDL_GetError()<<endl;
		        ret = AVERROR(ENOMEM);
		        goto fail;
		    }
		
		    memset(st_index, -1, sizeof(st_index));
		    is->last_video_stream = is->video_stream = -1;
		    is->last_audio_stream = is->audio_stream = -1;
		    is->last_subtitle_stream = is->subtitle_stream = -1;
		    is->eof = 0;
			
		    ic = avformat_alloc_context();
		    if (!ic) {
		    	PEF_DD[2]<<"Could not allocate context!"<<endl;
		        ret = AVERROR(ENOMEM);
		        goto fail;
		    }
		    ic->interrupt_callback.callback = decode_interrupt_cb;
		    ic->interrupt_callback.opaque = is;
		    if (!av_dict_get(format_opts, "scan_all_pmts", NULL, AV_DICT_MATCH_CASE)) {
		        av_dict_set(&format_opts, "scan_all_pmts", "1", AV_DICT_DONT_OVERWRITE);
		        scan_all_pmts_set = 1;
		    }
		    err = avformat_open_input(&ic, is->filename, is->iformat, &format_opts);
		    if (err < 0) {
		    	PEF_DD[2]<<"av_format_open_input "<<is->filename<<" Error: "<<err<<endl;
		        ret = -1;
		        goto fail;
		    }
		    if (scan_all_pmts_set)
		        av_dict_set(&format_opts, "scan_all_pmts", NULL, AV_DICT_MATCH_CASE);
		
		    if ((t = av_dict_get(format_opts, "", NULL, AV_DICT_IGNORE_SUFFIX))) {
		        PEF_DD[2]<<"Option "<<t->key<<" not found!"<<endl;
		        ret = AVERROR_OPTION_NOT_FOUND;
		        goto fail;
		    }
		    is->ic = ic;
		
		    if (genpts)
		        ic->flags |= AVFMT_FLAG_GENPTS;
		
		    av_format_inject_global_side_data(ic);
		
		    if (find_stream_info) {
		        AVDictionary **opts =setup_find_stream_info_opts(ic, codec_opts);
		        int orig_nb_streams = ic->nb_streams;
		
		        err = avformat_find_stream_info(ic, opts);
		        
		        {
		        	AVDictionaryEntry *tag=NULL;
					while ((tag=av_dict_get(ic->metadata,"",tag,AV_DICT_IGNORE_SUFFIX)))
						MediaMetaData[Atoa(tag->key)]=tag->value;
				}
		
		        for (i = 0; i < orig_nb_streams; i++)
		            av_dict_free(&opts[i]);
		        av_freep(&opts);
		
		        if (err < 0) {
		        	PEF_DD[1]<<is->filename<<" could not find codec parameters!"<<endl;
		            ret = -1;
		            goto fail;
		        }
		    }
		
		    if (ic->pb)
		        ic->pb->eof_reached = 0; // FIXME hack, ffplay maybe should not use avio_feof() to test for the end
		
		    if (seek_by_bytes < 0)
		        seek_by_bytes = !!(ic->iformat->flags & AVFMT_TS_DISCONT) && strcmp("ogg", ic->iformat->name);
		
		    is->max_frame_duration = (ic->iformat->flags & AVFMT_TS_DISCONT) ? 10.0 : 3600.0;
		
//		    if (!window_title && (t = av_dict_get(ic->metadata, "title", NULL, 0)))
//		        window_title = av_asprintf("%s - %s", t->value, input_filename);
		
		    /* if seeking requested, we execute it */
		    if (start_time != AV_NOPTS_VALUE) {
		        int64_t timestamp;
		
		        timestamp = start_time;
		        /* add the stream start time */
		        if (ic->start_time != AV_NOPTS_VALUE)
		            timestamp += ic->start_time;
		        ret = avformat_seek_file(ic, -1, (-9223372036854775807LL - 1), timestamp, 9223372036854775807LL, 0);
		        if (ret < 0) {
		        	PEF_DD[1]<<is->filename<<" could not seek to position "<<(double)timestamp / AV_TIME_BASE<<endl;
		        }
		    }
		
		    is->realtime = is_realtime(ic);
		
//		    if (show_status)
//		        av_dump_format(ic, 0, is->filename, 0);
		
		    for (i = 0; i < ic->nb_streams; i++) {
		        AVStream *st = ic->streams[i];
		        enum AVMediaType type = st->codecpar->codec_type;
		        st->discard = AVDISCARD_ALL;
		        if (type >= 0 && wanted_stream_spec[type] && st_index[type] == -1)
		            if (avformat_match_stream_specifier(ic, st, wanted_stream_spec[type]) > 0)
		                st_index[type] = i;
		    }
		    for (i = 0; i < AVMEDIA_TYPE_NB; i++) {
		        if (wanted_stream_spec[i] && st_index[i] == -1) {
		            PEF_DD[2]<<"Stream specifier "<<wanted_stream_spec[i]<<" does not match any "<<av_get_media_type_string((AVMediaType)i)<<" stream!"<<endl;
		            st_index[i] = INT_MAX;
		        }
		    }
		
		    if (!video_disable)
		        st_index[AVMEDIA_TYPE_VIDEO] =
		            av_find_best_stream(ic, AVMEDIA_TYPE_VIDEO,
		                                st_index[AVMEDIA_TYPE_VIDEO], -1, NULL, 0);
		    if (!audio_disable)
		        st_index[AVMEDIA_TYPE_AUDIO] =
		            av_find_best_stream(ic, AVMEDIA_TYPE_AUDIO,
		                                st_index[AVMEDIA_TYPE_AUDIO],
		                                st_index[AVMEDIA_TYPE_VIDEO],
		                                NULL, 0);
		    if (!video_disable && !subtitle_disable)
		        st_index[AVMEDIA_TYPE_SUBTITLE] =
		            av_find_best_stream(ic, AVMEDIA_TYPE_SUBTITLE,
		                                st_index[AVMEDIA_TYPE_SUBTITLE],
		                                (st_index[AVMEDIA_TYPE_AUDIO] >= 0 ?
		                                 st_index[AVMEDIA_TYPE_AUDIO] :
		                                 st_index[AVMEDIA_TYPE_VIDEO]),
		                                NULL, 0);
		
		    is->show_mode = show_mode;
		    if (st_index[AVMEDIA_TYPE_VIDEO] >= 0) {
		        AVStream *st = ic->streams[st_index[AVMEDIA_TYPE_VIDEO]];
		        AVCodecParameters *codecpar = st->codecpar;
		        AVRational sar = av_guess_sample_aspect_ratio(ic, st, NULL);
		        if (codecpar->width)
		            set_default_window_size(codecpar->width, codecpar->height, sar);
		    }
		
		    /* open the streams */
		    if (st_index[AVMEDIA_TYPE_AUDIO] >= 0) {
		        stream_component_open(is, st_index[AVMEDIA_TYPE_AUDIO]);
		    }
		
		    ret = -1;
		    if (st_index[AVMEDIA_TYPE_VIDEO] >= 0) {
		        ret = stream_component_open(is, st_index[AVMEDIA_TYPE_VIDEO]);
		    }
		    if (is->show_mode == SHOW_MODE_NONE)
		        is->show_mode = ret >= 0 ? SHOW_MODE_VIDEO : SHOW_MODE_RDFT;
		
		    if (st_index[AVMEDIA_TYPE_SUBTITLE] >= 0) {
		        stream_component_open(is, st_index[AVMEDIA_TYPE_SUBTITLE]);
		    }
		
		    if (is->video_stream < 0 && is->audio_stream < 0) {
		    	PEF_DD[2]<<"Failed to open file "<<is->filename<<" or configure filtergraph"<<endl,
		        ret = -1;
		        goto fail;
		    }
		
		    if (infinite_buffer < 0 && is->realtime)
		        infinite_buffer = 1;
			
			PrereadIsOK=1;
			
		    for (;;) {
		        if (is->abort_request)
		            break;
		        if (is->paused != is->last_paused) {
		            is->last_paused = is->paused;
		            if (is->paused)
		                is->read_pause_return = av_read_pause(ic);
		            else
		                av_read_play(ic);
		        }
		#if CONFIG_RTSP_DEMUXER || CONFIG_MMSH_PROTOCOL
		        if (is->paused &&
		                (!strcmp(ic->iformat->name, "rtsp") ||
		                 (ic->pb && !strncmp(input_filename, "mmsh:", 5)))) {
		            /* wait 10 ms to avoid trying to get another packet */
		            /* XXX: horrible */
		            SDL_Delay(10);
		            continue;
		        }
		#endif
		        if (is->seek_req) {
		            int64_t seek_target = is->seek_pos;
		            int64_t seek_min    = is->seek_rel > 0 ? seek_target - is->seek_rel + 2: (-9223372036854775807LL - 1);
		            int64_t seek_max    = is->seek_rel < 0 ? seek_target - is->seek_rel - 2: 9223372036854775807LL;
		//FIXME the +-2 is due to rounding being not done in the correct direction in generation
		//      of the seek_pos/seek_rel variables
		
		            ret = avformat_seek_file(is->ic, -1, seek_min, seek_target, seek_max, is->seek_flags);
		            if (ret < 0) 
		            	PEF_DD[2]<<is->ic->url<<": error while seeking"<<endl;
		            else {
		                if (is->audio_stream >= 0) {
		                    packet_queue_flush(&is->audioq);
		                    packet_queue_put(&is->audioq, &flush_pkt);
		                }
		                if (is->subtitle_stream >= 0) {
		                    packet_queue_flush(&is->subtitleq);
		                    packet_queue_put(&is->subtitleq, &flush_pkt);
		                }
		                if (is->video_stream >= 0) {
		                    packet_queue_flush(&is->videoq);
		                    packet_queue_put(&is->videoq, &flush_pkt);
		                }
		                if (is->seek_flags & AVSEEK_FLAG_BYTE) {
		                   set_clock(&is->extclk, NAN, 0);
		                } else {
		                   set_clock(&is->extclk, seek_target / (double)AV_TIME_BASE, 0);
		                }
		            }
		            is->seek_req = 0;
		            is->queue_attachments_req = 1;
		            is->eof = 0;
		            if (is->paused)
		                step_to_next_frame(is);
		        }
		        if (is->queue_attachments_req) {
		            if (is->video_st && is->video_st->disposition & AV_DISPOSITION_ATTACHED_PIC) {
		                AVPacket copy = { 0 };
		                if ((ret = av_packet_ref(&copy, &is->video_st->attached_pic)) < 0)
		                    goto fail;
		                packet_queue_put(&is->videoq, &copy);
		                packet_queue_put_nullpacket(&is->videoq, is->video_stream);
		            }
		            is->queue_attachments_req = 0;
		        }
		
		        /* if the queue are full, no need to read more */
		        if (infinite_buffer<1 &&
		              (is->audioq.size + is->videoq.size + is->subtitleq.size > MAX_QUEUE_SIZE
		            || (stream_has_enough_packets(is->audio_st, is->audio_stream, &is->audioq) &&
		                stream_has_enough_packets(is->video_st, is->video_stream, &is->videoq) &&
		                stream_has_enough_packets(is->subtitle_st, is->subtitle_stream, &is->subtitleq)))) {
		            /* wait 10 ms */
		            SDL_LockMutex(wait_mutex);
		            SDL_CondWaitTimeout(is->continue_read_thread, wait_mutex, 10);
		            SDL_UnlockMutex(wait_mutex);
		            continue;
		        }
		        if (!is->paused &&
		            (!is->audio_st || (is->auddec.finished == is->audioq.serial && frame_queue_nb_remaining(&is->sampq) == 0)) &&
		            (!is->video_st || (is->viddec.finished == is->videoq.serial && frame_queue_nb_remaining(&is->pictq) == 0))) {
		            if (loop != 1 && (!loop || --loop)) {
		                stream_seek(is, start_time != AV_NOPTS_VALUE ? start_time : 0, 0, 0);
		            } else
					{
						if (reach_end_flag==0)
						{
							SDL_Event event;
					        event.type = _FF_REACH_END;
					        event.user.code=0;
					        event.user.data1=NULL;
					        event.user.data2=NULL;
					        SDL_PushEvent(&event);
					        reach_end_flag=1;
						}
							
						if (autoexit) {
		                    ret = AVERROR_EOF;
		                    goto fail;
		                }
					} 
		        }
		        ret = av_read_frame(ic, pkt);
		        if (ret < 0) {
		            if ((ret == AVERROR_EOF || avio_feof(ic->pb)) && !is->eof) {
		                if (is->video_stream >= 0)
		                    packet_queue_put_nullpacket(&is->videoq, is->video_stream);
		                if (is->audio_stream >= 0)
		                    packet_queue_put_nullpacket(&is->audioq, is->audio_stream);
		                if (is->subtitle_stream >= 0)
		                    packet_queue_put_nullpacket(&is->subtitleq, is->subtitle_stream);
		                is->eof = 1;
		            }
		            if (ic->pb && ic->pb->error)
		                break;
		            SDL_LockMutex(wait_mutex);
		            SDL_CondWaitTimeout(is->continue_read_thread, wait_mutex, 10);
		            SDL_UnlockMutex(wait_mutex);
		            continue;
		        } else {
		            is->eof = 0;
		        }
		        /* check if packet is in play range specified by user, then queue, otherwise discard */
		        stream_start_time = ic->streams[pkt->stream_index]->start_time;
		        pkt_ts = pkt->pts == AV_NOPTS_VALUE ? pkt->dts : pkt->pts;
		        pkt_in_play_range = duration == AV_NOPTS_VALUE ||
		                (pkt_ts - (stream_start_time != AV_NOPTS_VALUE ? stream_start_time : 0)) *
		                av_q2d(ic->streams[pkt->stream_index]->time_base) -
		                (double)(start_time != AV_NOPTS_VALUE ? start_time : 0) / 1000000
		                <= ((double)duration / 1000000);
		        if (pkt->stream_index == is->audio_stream && pkt_in_play_range) {
		            packet_queue_put(&is->audioq, pkt);
		        } else if (pkt->stream_index == is->video_stream && pkt_in_play_range
		                   && !(is->video_st->disposition & AV_DISPOSITION_ATTACHED_PIC)) {
		            packet_queue_put(&is->videoq, pkt);
		        } else if (pkt->stream_index == is->subtitle_stream && pkt_in_play_range) {
		            packet_queue_put(&is->subtitleq, pkt);
		        } else {
		            av_packet_unref(pkt);
		        }
		        reach_end_flag=0;
		    }
		
		    ret = 0;
		 fail:
		 	PrereadIsOK=-1;
		    if (ic && !is->ic)
		        avformat_close_input(&ic);
		
		    if (ret != 0) {
		        SDL_Event event;
		
		        event.type = _FF_QUIT_EVENT;
		        event.user.code=0;
		        event.user.data1 = is;
		        event.user.data2=NULL;
		        SDL_PushEvent(&event);
		    }
		    SDL_DestroyMutex(wait_mutex);
		    return 0;
		}
		
		VideoState *stream_open(const char *filename,AVInputFormat *iformat)
		{
		    VideoState *is;
			
		    is =(VideoState*) av_mallocz(sizeof(VideoState));
		    if (!is)
		        return NULL;
		    is->filename = av_strdup(filename);
		    if (!is->filename)
		        goto fail;
		    is->iformat = iformat;
		    is->ytop    = 0;
		    is->xleft   = 0;
		
		    /* start video display */
		    if (frame_queue_init(&is->pictq, &is->videoq, VIDEO_PICTURE_QUEUE_SIZE, 1) < 0)
		        goto fail;
		    if (frame_queue_init(&is->subpq, &is->subtitleq, SUBPICTURE_QUEUE_SIZE, 0) < 0)
		        goto fail;
		    if (frame_queue_init(&is->sampq, &is->audioq, SAMPLE_QUEUE_SIZE, 1) < 0)
		        goto fail;
			
		    if (packet_queue_init(&is->videoq) < 0 ||
		        packet_queue_init(&is->audioq) < 0 ||
		        packet_queue_init(&is->subtitleq) < 0)
		        goto fail;
		
		    if (!(is->continue_read_thread = SDL_CreateCond())) {
		    	PEF_DD[2]<<"SDL_CreateCond Error: "<<SDL_GetError()<<endl;
		        goto fail;
		    }
		
		    init_clock(&is->vidclk, &is->videoq.serial);
		    init_clock(&is->audclk, &is->audioq.serial);
		    init_clock(&is->extclk, &is->extclk.serial);
		    is->audio_clock_serial = -1;
		    if (startup_volume < 0)
		    	PEF_DD[1]<<"volume="<<startup_volume<<" < 0, setting to 0"<<endl;
		    if (startup_volume > 100)
		        PEF_DD[1]<<"volume="<<startup_volume<<" > 100, setting to 100"<<endl;
		    startup_volume = av_clip(startup_volume, 0, 100);
		    startup_volume = av_clip(SDL_MIX_MAXVOLUME * startup_volume / 100, 0, SDL_MIX_MAXVOLUME);
		    is->audio_volume = startup_volume;
		    is->muted = 0;
		    is->av_sync_type = av_sync_type;
		    is->read_tid     = SDL_CreateThread(read_thread, "PAL_EasyFFmpeg_ReadThread", is);
		    if (!is->read_tid) 
			{
				PEF_DD[2]<<"SDL_CreateThread Error: "<<SDL_GetError()<<endl;
			fail:
		        stream_close(is);
		        return NULL;
		    }
		    return is;
		}
		
		void stream_cycle_channel(VideoState *is, int codec_type)
		{
		    AVFormatContext *ic = is->ic;
		    int start_index, stream_index;
		    int old_index;
		    AVStream *st;
		    AVProgram *p = NULL;
		    int nb_streams = is->ic->nb_streams;
		
		    if (codec_type == AVMEDIA_TYPE_VIDEO) {
		        start_index = is->last_video_stream;
		        old_index = is->video_stream;
		    } else if (codec_type == AVMEDIA_TYPE_AUDIO) {
		        start_index = is->last_audio_stream;
		        old_index = is->audio_stream;
		    } else {
		        start_index = is->last_subtitle_stream;
		        old_index = is->subtitle_stream;
		    }
		    stream_index = start_index;
		
		    if (codec_type != AVMEDIA_TYPE_VIDEO && is->video_stream != -1) {
		        p = av_find_program_from_stream(ic, NULL, is->video_stream);
		        if (p) {
		            nb_streams = p->nb_stream_indexes;
		            for (start_index = 0; start_index < nb_streams; start_index++)
		                if (p->stream_index[start_index] == stream_index)
		                    break;
		            if (start_index == nb_streams)
		                start_index = -1;
		            stream_index = start_index;
		        }
		    }
		
		    for (;;) {
		        if (++stream_index >= nb_streams)
		        {
		            if (codec_type == AVMEDIA_TYPE_SUBTITLE)
		            {
		                stream_index = -1;
		                is->last_subtitle_stream = -1;
		                goto the_end;
		            }
		            if (start_index == -1)
		                return;
		            stream_index = 0;
		        }
		        if (stream_index == start_index)
		            return;
		        st = is->ic->streams[p ? p->stream_index[stream_index] : stream_index];
		        if (st->codecpar->codec_type == codec_type) {
		            /* check that parameters are OK */
		            switch (codec_type) {
		            case AVMEDIA_TYPE_AUDIO:
		                if (st->codecpar->sample_rate != 0 &&
		                    st->codecpar->channels != 0)
		                    goto the_end;
		                break;
		            case AVMEDIA_TYPE_VIDEO:
		            case AVMEDIA_TYPE_SUBTITLE:
		                goto the_end;
		            default:
		                break;
		            }
		        }
		    }
		 the_end:
		    if (p && stream_index != -1)
		        stream_index = p->stream_index[stream_index];
		    	PEF_DD[0]<<"Switch "<<av_get_media_type_string((AVMediaType)codec_type)<<" stream from #"<<old_index<<" to #"<<stream_index<<endl;
		
		    stream_component_close(is, old_index);
		    stream_component_open(is, stream_index);
		}
		
		void seek_chapter(VideoState *is, int incr)
		{
//			int64_t pos = get_master_clock(is) * AV_TIME_BASE;
//			int i;
//			
//			if (!is->ic->nb_chapters)
//			return;
//				
//			/* find the current chapter */
//			for (i = 0; i < is->ic->nb_chapters; i++)
//			{
//				AVChapter *ch = is->ic->chapters[i];
//		    	if (av_compare_ts(pos, AV_TIME_BASE_Q, ch->start, ch->time_base) < 0) {
//		    		i--;
//					break;
//				}
//			}
//			
//			i += incr;
//			i = FFMAX(i, 0);
//			if (i >= is->ic->nb_chapters)
//				return;
//			
//			av_log(NULL, AV_LOG_VERBOSE, "Seeking to chapter %d.\n", i);
//			stream_seek(is, av_rescale_q(is->ic->chapters[i]->start, is->ic->chapters[i]->time_base,
//		                                 AV_TIME_BASE_Q), 0, 0);
		}
		
	};//End of namespace _PAL_EasyFFmpeg;
	
	
	inline Uint32 FF_QUIT_EVENT()
	{return _PAL_EasyFFmpeg::_FF_QUIT_EVENT;}
	
	inline Uint32 FF_REACH_END()
	{return _PAL_EasyFFmpeg::_FF_REACH_END;}
	
	/*
		3:case SDLK_a:
            stream_cycle_channel(cur_stream, AVMEDIA_TYPE_AUDIO);
            break;
        case SDLK_v:
            stream_cycle_channel(cur_stream, AVMEDIA_TYPE_VIDEO);
            break;
        case SDLK_c:
            stream_cycle_channel(cur_stream, AVMEDIA_TYPE_VIDEO);
            stream_cycle_channel(cur_stream, AVMEDIA_TYPE_AUDIO);
            stream_cycle_channel(cur_stream, AVMEDIA_TYPE_SUBTITLE);
            break;
        case SDLK_t:
            stream_cycle_channel(cur_stream, AVMEDIA_TYPE_SUBTITLE);
            break;
        
		4:case SDLK_w:
            toggle_audio_display(cur_stream);
            break;
	*/
	
	int RefreshWaitEvent(SDL_Event *event)//Use it in the main thread!;return 0:No refresh(use it as SDL_WaitEvent 1:Video 2:Video with subtitle 3:AudioDisplay
	{
		using namespace _PAL_EasyFFmpeg;
		double remaining_time=0.0;
		SDL_PumpEvents();
		while (!SDL_PeepEvents(event,1,SDL_GETEVENT,SDL_FIRSTEVENT,SDL_LASTEVENT))
		{
		    if (remaining_time>0.0)
		        av_usleep((int64_t)(remaining_time*1000000.0));
		    remaining_time=REFRESH_RATE;
		    if (videostate!=NULL&&videostate->show_mode!=SHOW_MODE_NONE&&(!videostate->paused||videostate->force_refresh))
		    {
				int re=video_refresh(videostate,&remaining_time);
		    	if (re>=1&&re<=3)
		    		return re;
			}
		    SDL_PumpEvents();
		}
		return 0;
	}
	
	inline void ForceRefresh()
	{_PAL_EasyFFmpeg::videostate->force_refresh=1;}
	
	inline SDL_Texture *GetVideoTexture()
	{
		using namespace _PAL_EasyFFmpeg;
		if (videostate)
			return videostate->vid_texture;
		else return NULL;
	}
	
	inline SDL_Texture *GetAudioDisplayTexture()
	{
		using namespace _PAL_EasyFFmpeg;
		if (videostate)
			return videostate->vis_texture;
		else return NULL;
	}
	inline SDL_Texture *GetSubtitleTexture()
	{
		using namespace _PAL_EasyFFmpeg;
		if (videostate)
			return videostate->sub_texture;
		else return NULL;
	}
	
	void ResizeDisplaySize(int w,int h)//It seems of no use...
	{
		using namespace _PAL_EasyFFmpeg;
		DisplaySize_W=w;
		DisplaySize_H=h;
//      screen_width  = cur_stream->width  = event.window.data1;
//      screen_height = cur_stream->height = event.window.data2;
		if (videostate->vis_texture)
		{
			SDL_DestroyTexture(videostate->vis_texture);
            videostate->vis_texture=NULL;
		}
	}
	
	void CycleAudioChannel()
	{
		using namespace _PAL_EasyFFmpeg;
		stream_cycle_channel(videostate,AVMEDIA_TYPE_AUDIO);
	}
	
	double GetDuration()//in seconds
	{
		using namespace _PAL_EasyFFmpeg;
		if (videostate==NULL||videostate->ic==NULL)
			return 0;
		return videostate->ic->duration / 1000000LL;
	}
	
	double GetCurrentPlayPos()//in seconds
	{
		using namespace _PAL_EasyFFmpeg;
		if (videostate==NULL)
			return 0;
		return videostate->audio_clock;
	}
	
	double GetCurrentPlayPercent()
	{
		using namespace _PAL_EasyFFmpeg;
		if (videostate==NULL||videostate->ic==NULL||videostate->ic->duration==0)
			return 0;
		return videostate->audio_clock*1000000LL/videostate->ic->duration;
	}
	
	void SeekTo(double per)//0~1
	{
		using namespace _PAL_EasyFFmpeg;
		if (videostate==NULL||videostate->ic==NULL)
			return;
		per=EnsureInRange(per,0,1);
		if (seek_by_bytes || videostate->ic->duration <= 0) 
		{
			uint64_t size =  avio_size(videostate->ic->pb);
			stream_seek(videostate, size*per, 0, 1);
		} 
		else 
		{
			double frac;
			int64_t ts;
			int ns, hh, mm, ss;
			int tns, thh, tmm, tss;
			tns  = videostate->ic->duration / 1000000LL;
			thh  = tns / 3600;
			tmm  = (tns % 3600) / 60;
			tss  = (tns % 60);
			frac = per;
			ns   = frac * tns;
			hh   = ns / 3600;
			mm   = (ns % 3600) / 60;
			ss   = (ns % 60);
			PEF_DD[0]<<"Seek to "<<frac*100<<"%"<<" ("<<hh<<":"<<mm<<":"<<ss<<") of total duration ("<<thh<<":"<<tmm<<":"<<tss<<")"<<endl;
			ts = frac * videostate->ic->duration;
			if (videostate->ic->start_time != AV_NOPTS_VALUE)
			    ts += videostate->ic->start_time;
			stream_seek(videostate, ts, 0, 0);
		}
	}
	
	void Seek(double delta=5)
	{
		using namespace _PAL_EasyFFmpeg;
		if (videostate==NULL||videostate->ic==NULL)
			return;
		double incr=delta,pos;
		if (seek_by_bytes)
		{
			pos = -1;
			if (pos < 0 && videostate->video_stream >= 0)
				pos = frame_queue_last_pos(&videostate->pictq);
			if (pos < 0 && videostate->audio_stream >= 0)
				pos = frame_queue_last_pos(&videostate->sampq);
			if (pos < 0)
				pos = avio_tell(videostate->ic->pb);
			if (videostate->ic->bit_rate)
				incr *= videostate->ic->bit_rate / 8.0;
			else
				incr *= 180000.0;
			pos += incr;
			stream_seek(videostate, pos, incr, 1);
		}
		else
		{
			pos = get_master_clock(videostate);
			if (std::isnan(pos))
				pos = (double)videostate->seek_pos / AV_TIME_BASE;
			pos += incr;
			if (videostate->ic->start_time != AV_NOPTS_VALUE && pos < videostate->ic->start_time / (double)AV_TIME_BASE)
				pos = videostate->ic->start_time / (double)AV_TIME_BASE;
			stream_seek(videostate, (int64_t)(pos * AV_TIME_BASE), (int64_t)(incr * AV_TIME_BASE), 0);
		}
	}
	
	void SeekPre()
	{Seek(-5);}
	
	void SeekNxt()
	{Seek(5);}
	
	int SeekChapter()
	{
//            if (cur_stream->ic->nb_chapters > 1)
//            	seek_chapter(cur_stream, 1);
//            if (cur_stream->ic->nb_chapters > 1) {
//            	seek_chapter(cur_stream, -1);
		return 0;
	}
	
	bool GetMuteState()
	{
		using namespace _PAL_EasyFFmpeg;
		if (videostate!=NULL)
			return videostate->muted;
		return 1;
	}
	
	bool Mute()
	{
		using namespace _PAL_EasyFFmpeg;
		if (videostate==NULL)
			return 1;
		toggle_mute(videostate);
		return videostate->muted;
	}

	void Mute(bool mute)
	{
		using namespace _PAL_EasyFFmpeg;
		if (videostate!=NULL&&videostate->muted^mute)
			toggle_mute(videostate);
	}
	
	bool GetPauseState()
	{
		using namespace _PAL_EasyFFmpeg;
		if (videostate!=NULL)
			return videostate->paused;
		return 1;
	}
	
	bool Pause()
	{
		using namespace _PAL_EasyFFmpeg;
		if (videostate==NULL)
			return 1;
		toggle_pause(videostate);
		return videostate->paused;
	}
	
	void Pause(bool pause)
	{
		using namespace _PAL_EasyFFmpeg;
		if (videostate!=NULL&&videostate->paused^pause)
			toggle_pause(videostate);
	}
	
	void StepToNextFrame()
	{
		using namespace _PAL_EasyFFmpeg;
		if (videostate!=NULL)
			step_to_next_frame(videostate);
	}
	
	int GetVolume()
	{
		using namespace _PAL_EasyFFmpeg;
		if (videostate!=NULL)
			return videostate->audio_volume/128.0*100;
		return 0;
	}
	
	void SetVolume(int vol)//0~100
	{
		using namespace _PAL_EasyFFmpeg;
		if (videostate!=NULL)
			videostate->audio_volume=EnsureInRange(vol/100.0*128,0,128);
	}
	
	int SetVolumePercent(double per)//0~1
	{
		using namespace _PAL_EasyFFmpeg;
		if (videostate!=NULL)
			return (videostate->audio_volume=EnsureInRange(per,0,1)*128)/128.0*100;
		return 0;
	}
	
	int SetVolumeDelta(int delta)//0~128
	{
		using namespace _PAL_EasyFFmpeg;
		if (videostate!=NULL)
			return (videostate->audio_volume=EnsureInRange(videostate->audio_volume+delta,0,128))/128.0*100;
		return 0;
	}
	
	int VolumeUp()
	{
		using namespace _PAL_EasyFFmpeg;
		if (videostate!=NULL)
		{
			update_volume(videostate,1,SDL_VOLUME_STEP);
			return videostate->audio_volume/128.0*100;
		}
		return 0;
	}
	
	int VolumeDown()
	{
		using namespace _PAL_EasyFFmpeg;
		if (videostate!=NULL)
		{
			update_volume(videostate,-1,SDL_VOLUME_STEP);
			return videostate->audio_volume/128.0*100;
		}
		return 0;
	}
	
	const std::map <std::string,std::string>& GetMediaMetaData()
	{return _PAL_EasyFFmpeg::MediaMetaData;}
	
	int Close()
	{
		using namespace _PAL_EasyFFmpeg;
		do_exit(videostate);
		videostate=NULL;
		return 0; 
	}
	
	int Open(const std::string &srcfile,bool audioonly,int StartUpVolume)
	{
		using namespace _PAL_EasyFFmpeg;
		PEF_DD[0]<<"Open "<<srcfile<<"..."<<endl;
		if (srcfile.empty())
			return PEF_DD[2]<<"srcFile is empty!"<<endl,1;
		if (!Inited)
			return PEF_DD[2]<<"PAL_EasyFFmpeg is not inited!"<<endl,2;
		if (videostate!=NULL)
		{
			PEF_DD[1]<<"Current open is not closed!"<<endl;
			Close();
			videostate=NULL;
		}
		
		//Reset init data
		{
			AVInputFormat *file_iformat=NULL;
			audio_disable=0;
			video_disable=0;
			subtitle_disable=0;
			memset(wanted_stream_spec,0,sizeof wanted_stream_spec);
			seek_by_bytes=-1;
			display_disable=0;
			startup_volume=100;
			av_sync_type=AV_SYNC_AUDIO_MASTER;
			start_time=AV_NOPTS_VALUE;
			duration=AV_NOPTS_VALUE;
			fast=0;
			genpts=0;
			lowres=0;
			decoder_reorder_pts=-1;
			autoexit=0;
			loop=1;
			framedrop=-1;
			infinite_buffer=-1;
			show_mode=SHOW_MODE_NONE;
			audio_codec_name=NULL;
			subtitle_codec_name=NULL;
			video_codec_name=NULL;
			rdftspeed=0.02;
			autorotate=1;
			find_stream_info=1;
			audio_callback_time=0;
			audio_dev=0;
			videostate=NULL;
			codec_opts=NULL;
			format_opts=NULL;
			AudioOnly=0;
			DisplaySize_W=0,
			DisplaySize_H=0;
			MediaMetaData.clear();
		}
		
		PrereadIsOK=0;
			
		srcFile=srcfile;
		display_disable=video_disable=AudioOnly=audioonly;
//		audio_disable=0/1;
//		subtitle_disable=0/1;
//		wanted_stream_spec[AVMEDIA_TYPE_AUDIO/AVMEDIA_TYPE_VIDEO/AVMEDIA_TYPE_SUBTITLE]="str";
//		start_time=XXX;//seek to a given position in seconds
//		duration=XXX;//play  "duration" seconds of audio/video
//		seek_by_bytes=XXX;//seek by bytes 0=off 1=on -1=auto
		startup_volume=StartUpVolume;//set startup volume 0=min 100=max
//		file_iformat=av_find_input_format("XXX");//force format
//		fast=0/1;//non spec compliant optimizations
//		gentps=0/1;//generate pts
//		decoder_reorder_pts=XXX;//let decoder reorder pts 0=off 1=on -1=auto
//		lowres=XXX;//There isn't any annotation in ffplay source code,so I don't know what it is.
//		av_sync_type=XXX;//set audio-video sync. type (AV_SYNC_AUDIO_MASTER/AV_SYNC_VIDEO_MASTER/AV_SYNC_EXTERNAL_CLOCK)
//		autoexit=0/1;//exit at the end(the meaning of this arg is not so clear,so be careful!)
//		loop=XXX;//set number of times the playback shall be looped
//		framedrop=0/1;//drop frames when cpu is too slow
//		infinite_buffer=0/1;//don't limit the input buffer size (useful with realtime streams)
//		rdftspeed=XXX;//rdft speed(msecs)(I don't know what is this...)
//		show_mode=XXX;//select show mode (SHOW_MODE_VIDEO/SHOW_MODE_WAVES/SHOW_MODE_RDFT)
//		audio_codec_name="str";//force audio decoder
//		subtitle_codec_name="str";//force subtitle decoder
//		video_codec_name="str";//force video decoder
//		autorotate=0/1;//automatically rotate video
//		find_stream_info=0/1;//read and decode the streams to fill missing information with heuristics
		
	    av_init_packet(&flush_pkt);
	    flush_pkt.data=(uint8_t*)&flush_pkt;
		
	    if ((videostate=stream_open(srcFile.c_str(),file_iformat))==NULL)
			return PEF_DD[2]<<"Failed to init VideoState!"<<endl,3;
		
		while (PrereadIsOK!=1)
			if (PrereadIsOK==-1)
				return 4;
			else SDL_Delay(1);
		
		PEF_DD[0]<<"Open "<<srcfile<<" OK."<<endl;
	    return 0;
	}
	
	int Quit()
	{
		using namespace _PAL_EasyFFmpeg;
		PEF_DD[0]<<"PAL_EasyFFmpeg_Quit..."<<endl;
		Close();
		avformat_network_deinit();
		Inited=0;
		PEF_DD[0]<<"PAL_EasyFFmpeg_Quit OK."<<endl;
		return 0;
	}
	
	int Init(SDL_Renderer *ren)//SDL2 should be inited before call this.
	{
		using namespace _PAL_EasyFFmpeg;
//		PEF_DD.SetLOGFile("PAL_EasyFFmpeg_DebugLog.txt");
//		PEF_DD%DebugOut_CERR_LOG;
		PEF_DD.SetDebugType(0,"PAL_EasyFFmpeg_Info");
		PEF_DD.SetDebugType(1,"PAL_EasyFFmpeg_Warning");
		PEF_DD.SetDebugType(2,"PAL_EasyFFmpeg_Error");
		PEF_DD.SetDebugType(3,"PAL_EasyFFmpeg_Debug");
		
		PEF_DD[0]<<"PAL_EasyFFmpeg_Init..."<<endl;
		
		int ret=0;
		av_register_all();
		if (ret=avformat_network_init())
			return PEF_DD[2]<<"avformat_network_init failed: "<<ret<<"!"<<endl,1;

        /* Try to work around an occasional ALSA buffer underflow issue when the
         * period size is NPOT due to ALSA resampling by forcing the buffer size. */
        if (!SDL_getenv("SDL_AUDIO_ALSA_SET_BUFFER_SIZE"))
            SDL_setenv("SDL_AUDIO_ALSA_SET_BUFFER_SIZE","1", 1);
		
		renderer=ren;
		
		if ((_FF_QUIT_EVENT=SDL_RegisterEvents(1))==0xffffffff)
			return PEF_DD[2]<<"Cannot register FF_QUIT_EVENT!"<<endl,2;
		
		if ((_FF_REACH_END=SDL_RegisterEvents(1))==0xffffffff)
			return PEF_DD[2]<<"Cannot register FF_REACH_END!"<<endl,3;
		
		Inited=1;
		PEF_DD[0]<<"PAL_EasyFFmpeg_Init OK."<<endl;
		return 0;
	}
	
	struct MediaInfo
	{
		std::map <std::string,std::string> metadata;
		SDL_Surface *sur=NULL;
	};
	
	MediaInfo* GetMediaInfo(const std::string &src,bool NeedCover=1)
	{
		using namespace _PAL_EasyFFmpeg;
		AVFormatContext	*pFormatCtx_info=avformat_alloc_context();
		
		if(avformat_open_input(&pFormatCtx_info,src.c_str(),NULL,NULL)!=0)
		{
			PEF_DD[2]<<"GetMediaInfo:Couldn't open input stream!"<<endl;
			avformat_close_input(&pFormatCtx_info);
			return NULL;
		}
	
		if(avformat_find_stream_info(pFormatCtx_info,NULL)<0)
		{
			PEF_DD[2]<<"GetMediaInfo:Couldn't find stream information!"<<endl;
			avformat_close_input(&pFormatCtx_info);
			return NULL;
		}

	//	av_dump_format(pFormatCtx, 0, url, false);
		
		MediaInfo *re=new MediaInfo();
		
		AVDictionaryEntry *tag=NULL;
		while ((tag=av_dict_get(pFormatCtx_info->metadata,"",tag,AV_DICT_IGNORE_SUFFIX)))
			re->metadata[Atoa(tag->key)]=tag->value;
		
		if (NeedCover)
			for (int i =0;i<pFormatCtx_info->nb_streams;++i)
				if (pFormatCtx_info->streams[i]->disposition&AV_DISPOSITION_ATTACHED_PIC) 
				{
					AVPacket pkt=pFormatCtx_info->streams[i]->attached_pic;
		            re->sur=IMG_Load_RW(SDL_RWFromConstMem((unsigned char*)pkt.data,pkt.size),1);
					break;
				}
		
		avformat_close_input(&pFormatCtx_info);
		return re;	
	}
};
#endif
