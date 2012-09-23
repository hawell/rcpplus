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

#include "rcpdefs.h"
#include "rcpplus.h"
#include "rcpcommand.h"
#include "rtp.h"
#include "rcplog.h"
#include "coder.h"

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavfilter/avfilter.h"
#include "libavutil/avutil.h"
#include "libswscale/swscale.h"

//#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#define MAX_FRAME_LEN	100000

unsigned char sbit_mask[] = {0xFF, 0x7F, 0x3F, 0x1F, 0x0F, 0x07, 0x03, 0x01, 0x00};
unsigned char ebit_mask[] = {0xFF, 0xFE, 0xFC, 0xF8, 0xF0, 0xE0, 0xC0, 0x80, 0x00};

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

int main()
{
	rcplog_init(LOG_MODE_STDERR, LOG_INFO, NULL);

	av_register_all();

	AVCodec* codec_in = avcodec_find_decoder(CODEC_ID_H263);
	if (codec_in == NULL)
	{
		ERROR("cannot find decoder");
		return -1;
	}

	AVCodecContext* codec_ctx_in = avcodec_alloc_context3(codec_in);
	if (codec_ctx_in == NULL)
	{
		ERROR("cannot allocate codec context");
		return -1;
	}

	if (avcodec_open2(codec_ctx_in, codec_in, NULL) < 0)
	{
		ERROR("cannot open codec");
		return -1;
	}

	codec_ctx_in->width = 704;
	codec_ctx_in->height = 576;
	codec_ctx_in->pix_fmt = PIX_FMT_YUV420P;

	AVFrame* raw_frame = avcodec_alloc_frame();
	if (raw_frame == NULL)
	{
		ERROR("cannot allocate frame");
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

	enum PixelFormat pf;
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
		ERROR("Could not find suitable output format");
		return -1;
	}

	of->video_codec = CODEC_ID_H263;

	ofc = (AVFormatContext *)av_mallocz(sizeof(AVFormatContext));
	if ( !ofc )
	{
		ERROR("Memory error");
		return -1;
	}
	ofc->oformat = of;

	pf = PIX_FMT_RGB24;
	ost_v = avformat_new_stream(ofc, NULL);
	if (!ost_v)
	{
		ERROR("Could not alloc stream");
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
		INFO("h264");
		c->me_range = 16;
		c->max_qdiff = 4;
		c->qmin = 10;
		c->qmax = 26;
		c->qcompress = 0.6;
	}

	AVCodec *codec = avcodec_find_encoder(c->codec_id);
	if ( !codec )
	{
		ERROR("codec not found");
		return -1;
	}
	if ( avcodec_open2(c, codec, NULL) < 0 )
	{
		ERROR("Could not open codec");
		return -1;
	}

	/* allocate the encoded raw picture */
	opicture = avcodec_alloc_frame();
	if ( !opicture )
	{
		ERROR("Could not allocate opicture");
		return -1;
	}
	int size = avpicture_get_size( c->pix_fmt, c->width, c->height);
	uint8_t *opicture_buf = (uint8_t *)malloc(size);
	if ( !opicture_buf )
	{
		av_free(opicture);
		ERROR("Could not allocate opicture");
		return -1;
	}
	avpicture_fill( (AVPicture *)opicture, opicture_buf, c->pix_fmt, c->width, c->height );
	tmp_opicture = NULL;

	tmp_opicture = avcodec_alloc_frame();
	if ( !tmp_opicture )
	{
		ERROR("Could not allocate temporary opicture");
		return -1;
	}
	size = avpicture_get_size( PIX_FMT_YUV420P, c->width, c->height);
	uint8_t *tmp_opicture_buf = (uint8_t *)malloc(size);
	if (!tmp_opicture_buf)
	{
		av_free( tmp_opicture );
		ERROR("Could not allocate temporary opicture");
		return -1;
	}
	avpicture_fill( (AVPicture *)tmp_opicture, tmp_opicture_buf, PIX_FMT_YUV420P, c->width, c->height );

	if ( avio_open(&ofc->pb, fname, AVIO_FLAG_WRITE) < 0 )
	{
		ERROR("Could not open '%s'", fname);
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


	rcp_connect("10.25.25.223");

	rcp_session session;
	memset(&session, 0, sizeof(rcp_session));
	session.user_level = RCP_USER_LEVEL_LIVE;

	client_register(RCP_REGISTRATION_TYPE_NORMAL, RCP_ENCRYPTION_MODE_MD5, &session);

	get_capability_list(&session);

	rcp_coder_list encoders, decoders;
	get_coder_list(&session, RCP_CODER_ENCODER, RCP_MEDIA_TYPE_VIDEO, &encoders);
	DEBUG("***");
	for (int i=0; i<encoders.count; i++)
		DEBUG("%x %x %x %x %x", encoders.coder[i].number, encoders.coder[i].caps, encoders.coder[i].current_cap, encoders.coder[i].param_caps, encoders.coder[i].current_param);
	DEBUG("***");
	get_coder_list(&session, RCP_CODER_DECODER, RCP_MEDIA_TYPE_VIDEO, &decoders);
	DEBUG("***");
	for (int i=0; i<decoders.count; i++)
		DEBUG("%x %x %x %x %x", decoders.coder[i].number, decoders.coder[i].caps, decoders.coder[i].current_cap, decoders.coder[i].param_caps, decoders.coder[i].current_param);
	DEBUG("***");

	unsigned short udp_port = stream_connect_udp();

	DEBUG("udp port = %d", udp_port);

	rcp_media_descriptor desc = {
			RCP_MEP_UDP, 1, 1, 0, udp_port, 1, 1, RCP_VIDEO_CODING_MPEG4, RCP_VIDEO_RESOLUTION_4CIF
	};

	client_connect(&session, RCP_CONNECTION_METHOD_GET, RCP_MEDIA_TYPE_VIDEO, 0, &desc);

	int res = fork();
	if (res == 0)
	{
		while (1)
		{
			int n = keep_alive(&session);
			INFO("active connections = %d", n);
			if (n < 0)
				break;

			sleep(2);
		}
	}

	struct sockaddr_in si_remote;
	socklen_t slen = sizeof(si_remote);
	unsigned char buffer[1500];

	uint8_t complete_frame[MAX_FRAME_LEN];
	int complete_frame_size = 0;

	unsigned short last_seq = 0;
	int ebit = 0;
	time_t end_time = time(NULL) + 10;
	while (time(NULL) < end_time)
	{
		int num = recvfrom(con.stream_socket, buffer, 1500, 0, (struct sockaddr*)&si_remote, &slen);

		rtp_header *hdr = (rtp_header*)buffer;
		int header_len = RTP_HEADER_LENGTH_MANDATORY + hdr->cc*4;
		DEBUG("m=%d pt=%d hlen=%d len=%d seq=%d ts=%u", hdr->m, hdr->pt, header_len, num, hdr->seq, hdr->timestamp);

		last_seq = hdr->seq;

		h263_header *hdr2 = (h263_header*)(buffer+header_len);
		DEBUG("sbit=%d ebit=%d", hdr2->sbit, hdr2->ebit);
		unsigned char* pos = buffer+header_len;
		int len = num - header_len;
		if (hdr2->f == 0)	// type A
		{
			pos += 4;
			len -= 4;
		}
		else if (hdr2->p == 0)	// type B
		{
			pos += 8;
			len -= 8;
		}
		else	// type C
		{
			pos += 12;
			len -= 12;
		}
		//fwrite(pos, len, 1, stdout);

		DEBUG("%d %d", ebit, hdr2->sbit);
		assert((ebit + hdr2->sbit) % 8 == 0);
		if (hdr2->sbit)
		{
			complete_frame[complete_frame_size - 1] &= ebit_mask[ebit];
			pos[0] &= sbit_mask[hdr2->sbit];
			complete_frame[complete_frame_size - 1] |= pos[0];
			pos++;
			len--;
		}
		memcpy(complete_frame+complete_frame_size, pos, len);
		complete_frame_size += len;
		ebit = hdr2->ebit;

		if (hdr->m == 1) // end of frame
		{
			//DEBUG("writing %d bytes", complete_frame_size);
			//int ret = fwrite(complete_frame, complete_frame_size, 1, stdout);
			//DEBUG("written %d", ret);
			pts += hdr->timestamp;

			int have_frame=0;
			in_pkt.data = complete_frame;
			in_pkt.size = complete_frame_size;
			//ERROR("1");
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

			complete_frame_size = 0;
		}
	}

	av_write_trailer(ofc);
	kill(res, SIGTERM);

	return 0;
}
