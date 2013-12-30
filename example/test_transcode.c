/*
 * test_transcode.c
 *
 *  Created on: Sep 18, 2012
 *      Author: arash
 */

#define _POSIX_SOURCE

#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <signal.h>
#include <tlog/tlog.h>
#include <pthread.h>

#include "rcpdefs.h"
#include "rcpplus.h"
#include "rcpcommand.h"
#include "rtp.h"
#include "coder.h"

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
//#include "libavfilter/avfilter.h"
#include "libavutil/avutil.h"
#include "libswscale/swscale.h"

//#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

char* fname = "x.avi";

void save_frame(AVFrame *frame, int width, int height)
{
	static int fnum = 0;
	FILE *f;
	char fname[32];
	int  y;

	// Open file
	sprintf(fname, "frame%02d.ppm", fnum++);
	f = fopen(fname, "wb");
	if(f == NULL)
		return;

	// Write header
	fprintf(f, "P6\n%d %d\n255\n", width, height);

	// Write pixel data
	for (y=0; y<height; y++)
		fwrite(frame->data[0] + y*frame->linesize[0], 1, width*3, f);

	// Close file
	fclose(f);
}

pthread_t thread;
void* keep_alive_thread(void* params)
{
	rcp_session* session = (rcp_session*)params;
	while (1)
	{
		int n = keep_alive(session);
		TL_INFO("active connections = %d", n);
		if (n < 0)
			break;

		sleep(2);
	}
	return NULL;
}

int main(int argc, char* argv[])
{
	tlog_init(TLOG_MODE_STDERR, TLOG_INFO, NULL);

	if (argc < 2)
	{
		TL_INFO("transcode ip");
		return -1;
	}

	av_register_all();

	AVCodec* codec_in = avcodec_find_decoder(CODEC_ID_H263);
	if (codec_in == NULL)
	{
		TL_ERROR("cannot find decoder");
		return -1;
	}

	AVCodecContext* codec_ctx_in = avcodec_alloc_context3(codec_in);
	if (codec_ctx_in == NULL)
	{
		TL_ERROR("cannot allocate codec context");
		return -1;
	}

	if (avcodec_open2(codec_ctx_in, codec_in, NULL) < 0)
	{
		TL_ERROR("cannot open codec");
		return -1;
	}

	// TODO: change stream profile to 4CIF or adjust codec parameters

	codec_ctx_in->width = 704;
	codec_ctx_in->height = 576;
	codec_ctx_in->pix_fmt = PIX_FMT_YUV420P;

	AVFrame* raw_frame = avcodec_alloc_frame();
	if (raw_frame == NULL)
	{
		TL_ERROR("cannot allocate frame");
		return -1;
	}

	AVPacket in_pkt;
	av_init_packet(&in_pkt);

	AVFrame* rgb = avcodec_alloc_frame();
	int num_bytes = avpicture_get_size(PIX_FMT_RGB24, codec_ctx_in->width, codec_ctx_in->height);
	uint8_t* rgb_buffer = (uint8_t *)av_malloc(num_bytes*sizeof(uint8_t));
	avpicture_fill((AVPicture *)rgb, rgb_buffer, PIX_FMT_RGB24, codec_ctx_in->width, codec_ctx_in->height);

	struct SwsContext *yuv2rgb;
	yuv2rgb = sws_getContext(codec_ctx_in->width, codec_ctx_in->height, PIX_FMT_YUV420P,
			codec_ctx_in->width, codec_ctx_in->height, PIX_FMT_RGB24, SWS_BICUBIC, NULL, NULL, NULL);

	//***********************************************

	AVOutputFormat *of;
	AVFormatContext *ofc;
	AVStream *ost_v;
	AVFrame *opicture;
	AVFrame *tmp_opicture;
	uint8_t *video_outbuf;
	int video_outbuf_size;
	int64_t pts=0;

	of = av_guess_format( "avi", fname, NULL);
	if ( !of )
	{
		TL_ERROR("Could not find suitable output format");
		return -1;
	}

	of->video_codec = CODEC_ID_H263;

	ofc = (AVFormatContext *)av_mallocz(sizeof(AVFormatContext));
	if ( !ofc )
	{
		TL_ERROR("Memory error");
		return -1;
	}
	ofc->oformat = of;

	ost_v = avformat_new_stream(ofc, NULL);
	if (!ost_v)
	{
		TL_ERROR("Could not alloc stream");
		return -1;
	}
	AVCodecContext *c = ost_v->codec;
	avcodec_get_context_defaults3(c, AVMEDIA_TYPE_VIDEO);

	c->codec_id = of->video_codec;
	c->codec_type = AVMEDIA_TYPE_VIDEO;
	c->bit_rate = 1000000;
	c->pix_fmt = PIX_FMT_YUV420P;
	c->width = 704;
	c->height = 576;
	c->time_base.den = (int)(25*100);
	c->time_base.num = 100;
	c->gop_size = 25;
	c->me_method = ME_EPZS;
	c->mb_decision = FF_MB_DECISION_BITS;
	c->noise_reduction = 250;
	c->pre_me = 0;

	if (ofc->oformat->flags & AVFMT_GLOBALHEADER)
		c->flags |= CODEC_FLAG_GLOBAL_HEADER;

	if (c->codec_id == CODEC_ID_H264)
	{
		TL_INFO("h264");
		c->me_range = 16;
		c->max_qdiff = 4;
		c->qmin = 10;
		c->qmax = 26;
		c->qcompress = 0.6;
	}

	AVCodec *codec = avcodec_find_encoder(c->codec_id);
	if ( !codec )
	{
		TL_ERROR("codec not found");
		return -1;
	}
	if ( avcodec_open2(c, codec, NULL) < 0 )
	{
		TL_ERROR("Could not open codec");
		return -1;
	}

	/* allocate the encoded raw picture */
	opicture = avcodec_alloc_frame();
	if ( !opicture )
	{
		TL_ERROR("Could not allocate opicture");
		return -1;
	}
	int size = avpicture_get_size( c->pix_fmt, c->width, c->height);
	uint8_t *opicture_buf = (uint8_t *)malloc(size);
	if ( !opicture_buf )
	{
		av_free(opicture);
		TL_ERROR("Could not allocate opicture");
		return -1;
	}
	avpicture_fill( (AVPicture *)opicture, opicture_buf, c->pix_fmt, c->width, c->height );
	tmp_opicture = NULL;

	tmp_opicture = avcodec_alloc_frame();
	if ( !tmp_opicture )
	{
		TL_ERROR("Could not allocate temporary opicture");
		return -1;
	}
	size = avpicture_get_size( PIX_FMT_YUV420P, c->width, c->height);
	uint8_t *tmp_opicture_buf = (uint8_t *)malloc(size);
	if (!tmp_opicture_buf)
	{
		av_free( tmp_opicture );
		TL_ERROR("Could not allocate temporary opicture");
		return -1;
	}
	avpicture_fill( (AVPicture *)tmp_opicture, tmp_opicture_buf, PIX_FMT_YUV420P, c->width, c->height );

	if ( avio_open(&ofc->pb, fname, AVIO_FLAG_WRITE) < 0 )
	{
		TL_ERROR("Could not open '%s'", fname);
		return -1;
	}
	video_outbuf = NULL;
	if ( !(ofc->oformat->flags & AVFMT_RAWPICTURE) )
	{
		/* allocate output buffer */
		/* XXX: API change will be done */
		video_outbuf_size = 200000;
		video_outbuf = (uint8_t *)malloc(video_outbuf_size);
	}

	/* write the stream header, if any */
	avformat_write_header(ofc, NULL);

	av_dump_format(ofc, 0, "stdout", 1);


	rcp_connect(argv[1]);

	start_event_handler();

	client_register(RCP_USER_LEVEL_LIVE, "", RCP_REGISTRATION_TYPE_NORMAL, RCP_ENCRYPTION_MODE_MD5);

	rcp_coder_list encoders, decoders;
	get_coder_list(RCP_CODER_ENCODER, RCP_MEDIA_TYPE_VIDEO, &encoders);
	TL_DEBUG("***");
	for (int i=0; i<encoders.count; i++)
		TL_DEBUG("%x %x %x %x %x", encoders.coder[i].number, encoders.coder[i].caps, encoders.coder[i].current_cap, encoders.coder[i].param_caps, encoders.coder[i].current_param);
	TL_DEBUG("***");
	get_coder_list(RCP_CODER_DECODER, RCP_MEDIA_TYPE_VIDEO, &decoders);
	TL_DEBUG("***");
	for (int i=0; i<decoders.count; i++)
		TL_DEBUG("%x %x %x %x %x", decoders.coder[i].number, decoders.coder[i].caps, decoders.coder[i].current_cap, decoders.coder[i].param_caps, decoders.coder[i].current_param);
	TL_DEBUG("***");

	rcp_session session;
	memset(&session, 0, sizeof(rcp_session));
	unsigned short udp_port = stream_connect_udp(&session);

	TL_DEBUG("udp port = %d", udp_port);

	rcp_media_descriptor desc = {
			RCP_MEP_UDP, 1, 1, 0, udp_port, 1, 1, RCP_VIDEO_CODING_MPEG4, RCP_VIDEO_RESOLUTION_4CIF
	};

	client_connect(&session, RCP_CONNECTION_METHOD_GET, RCP_MEDIA_TYPE_VIDEO, 0, &desc);


	pthread_create(&thread, NULL, keep_alive_thread, &session);

	rtp_merge_desc mdesc;
	rtp_init(RTP_PAYLOAD_TYPE_H263, 1, &mdesc);
	video_frame vframe;

	time_t end_time = time(NULL) + 10;
	while (time(NULL) < end_time)
	{
/*
		int num = recvfrom(con.stream_socket, buffer, 1500, 0, (struct sockaddr*)&si_remote, &slen);

		rtp_push_frame(buffer, num, &mdesc);
*/
		rtp_recv(session.stream_socket, &mdesc);

		if (rtp_pop_frame(&vframe, &mdesc) == 0)
		{
			int have_frame=0;
			in_pkt.data = vframe.data;
			in_pkt.size = vframe.len;
			//TL_ERROR("1");
			int ret = avcodec_decode_video2(codec_ctx_in, raw_frame, &have_frame, &in_pkt);
			if (ret && have_frame)
			{
				sws_scale(yuv2rgb, (const uint8_t * const*)raw_frame->data, raw_frame->linesize, 0, codec_ctx_in->height, rgb->data, rgb->linesize);

				//save_frame(rgb, codec_ctx_in->width, codec_ctx_in->height);

				int got_packet = 0;
				AVPacket pkt;
				av_init_packet(&pkt);
				pkt.data = video_outbuf;
				pkt.size = video_outbuf_size;
				pkt.pts = AV_NOPTS_VALUE;
				if(c->coded_frame->key_frame)
					pkt.flags |= pts;
				pkt.stream_index = ost_v->index;
				avcodec_encode_video2(c, &pkt, raw_frame, &got_packet);
				if (got_packet)
				{
					//fprintf(stderr, "video pts : %lld\n", pkt.pts);
					ret = av_write_frame( ofc, &pkt );
				}
			}

		}
	}

	av_write_trailer(ofc);

	pthread_cancel(thread);

	client_disconnect(&session);

	client_unregister();

	stop_event_handler();

	return 0;
}
