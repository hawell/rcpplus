/*
 * test_play.c
 *
 *  Created on: Nov 7, 2012
 *      Author: arash
 */

#define _XOPEN_SOURCE	600

#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <signal.h>
#include <tlog/tlog.h>

#include "rcpdefs.h"
#include "rcpplus.h"
#include "rcpcommand.h"
#include "rtp.h"
#include "coder.h"
#include "preset.h"

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavfilter/avfilter.h"
#include "libavutil/avutil.h"
#include "libswscale/swscale.h"

#include <SDL/SDL.h>
#include <SDL/SDL_thread.h>

#define VIDEO_MODE_H263	1
#define VIDEO_MODE_H264	2

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

int main(int argc, char* argv[])
{
	tlog_init(TLOG_MODE_STDERR, TLOG_INFO, NULL);

	if (argc < 2)
	{
		INFO("%s ip\n", argv[0]);
		return 0;
	}

	rcp_connect(argv[1]);


	client_register(RCP_USER_LEVEL_LIVE, "", RCP_REGISTRATION_TYPE_NORMAL, RCP_ENCRYPTION_MODE_MD5);

	rcp_coder_list encoders;
	get_coder_list(RCP_CODER_DECODER, RCP_MEDIA_TYPE_VIDEO, &encoders);
	int coder_id = 1;
	int resolution = RCP_VIDEO_RESOLUTION_QCIF;
	for (int i=0; i<encoders.count; i++)
	{
		if ((encoders.coder[i].current_cap & RCP_VIDEO_CODING_H264) || (encoders.coder[i].current_cap & RCP_VIDEO_CODING_H263))
		{
			log_coder(TLOG_INFO, &encoders.coder[i]);
			coder_id = encoders.coder[i].number;
			resolution = encoders.coder[i].current_param;
			break;
		}
	}
	if (resolution && RCP_VIDEO_RESOLUTION_4CIF)
		resolution = RCP_VIDEO_RESOLUTION_4CIF;
	else if (resolution && RCP_VIDEO_RESOLUTION_2CIF)
		resolution = RCP_VIDEO_RESOLUTION_2CIF;
	else if (resolution && RCP_VIDEO_RESOLUTION_CIF)
		resolution = RCP_VIDEO_RESOLUTION_CIF;
	else if (resolution && RCP_VIDEO_RESOLUTION_QCIF)
		resolution = RCP_VIDEO_RESOLUTION_QCIF;

	int preset_id = get_coder_preset(coder_id);
	rcp_mpeg4_preset preset;
	get_preset(preset_id, &preset, 1);

	int video_mode;
	get_coder_video_operation_mode(coder_id, &video_mode);

	INFO("mode=%d res=%d id=%d", video_mode, resolution, coder_id);

	int coding;
	if (video_mode == VIDEO_MODE_H263)
		coding = RCP_VIDEO_CODING_MPEG4;
	else
		coding = RCP_VIDEO_CODING_H264;

	avcodec_register_all();
	av_register_all();
	avfilter_register_all();

	AVCodec* codec_in;

	rtp_merge_desc mdesc;
	video_frame vframe;

	if (video_mode == VIDEO_MODE_H263)
	{
		rtp_init(RTP_PAYLOAD_TYPE_H263, 1, &mdesc);
		codec_in = avcodec_find_decoder(CODEC_ID_H263);
	}
	else
	{
		rtp_init(RTP_PAYLOAD_TYPE_H264, 1, &mdesc);
		codec_in = avcodec_find_decoder(CODEC_ID_H264);
	}

	if (codec_in == NULL)
	{
		ERROR("cannot find decoder");
		return -1;
	}

	AVCodecContext *dec_ctx = avcodec_alloc_context3(codec_in);
	if (dec_ctx == NULL)
	{
		ERROR("cannot allocate codec context");
		return -1;
	}

	if (avcodec_open2(dec_ctx, codec_in, NULL) < 0)
	{
		ERROR("cannot open codec");
		return -1;
	}

	dec_ctx->width = 704;
	dec_ctx->height = 576;
	dec_ctx->pix_fmt = PIX_FMT_YUV420P;

	AVFrame* raw_frame = avcodec_alloc_frame();
	if (raw_frame == NULL)
	{
		ERROR("cannot allocate frame");
		return -1;
	}

	AVPacket in_pkt;
	av_init_packet(&in_pkt);

	AVFrame* rgb_frame = avcodec_alloc_frame();
	int num_bytes = avpicture_get_size(PIX_FMT_RGB24, dec_ctx->width, dec_ctx->height);
	uint8_t* rgb_buffer = (uint8_t *)av_malloc(num_bytes*sizeof(uint8_t));
	avpicture_fill((AVPicture *)rgb_frame, rgb_buffer, PIX_FMT_RGB24, dec_ctx->width, dec_ctx->height);

	struct SwsContext *yuv2rgb, *rgb2yuv;
	yuv2rgb = sws_getContext(dec_ctx->width, dec_ctx->height, PIX_FMT_YUV420P,
			dec_ctx->width, dec_ctx->height, PIX_FMT_RGB24, SWS_BICUBIC, NULL, NULL, NULL);
	rgb2yuv = sws_getContext(dec_ctx->width, dec_ctx->height, PIX_FMT_RGB24,
			dec_ctx->width, dec_ctx->height, PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);
	assert(rgb2yuv != NULL);

	//***********************************************

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER))
	{
		ERROR("could not initialize SDL - %s\n", SDL_GetError());
		return -1;
	}
	SDL_Surface *screen;

	screen = SDL_SetVideoMode(704, 576, 0, 0);
	if (!screen)
	{
		ERROR("could not initialize video mode");
		return -1;
	}

	SDL_Overlay *bmp;
	bmp = SDL_CreateYUVOverlay(704, 576, SDL_YV12_OVERLAY, screen);

	rcp_session session;
	memset(&session, 0, sizeof(rcp_session));
	unsigned short udp_port = stream_connect_udp(&session);
	rcp_media_descriptor desc = {
			RCP_MEP_UDP, 1, 1, 0, udp_port, coder_id, 1, coding, resolution
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

	while (1)
	{
		if (rtp_recv(session.stream_socket, &mdesc) == 0)
        {
		if (rtp_pop_frame(&vframe, &mdesc) == 0)
		{
			int have_frame=0;
			in_pkt.data = vframe.data;
			in_pkt.size = vframe.len;
			//ERROR("1");
			int ret = avcodec_decode_video2(dec_ctx, raw_frame, &have_frame, &in_pkt);
			if (ret && have_frame)
			{
				//INFO("d0 %d d1 %d d2 %d d3 %d", raw_frame->linesize[0], raw_frame->linesize[1], raw_frame->linesize[2], raw_frame->linesize[3]);
				sws_scale(yuv2rgb, (const uint8_t * const*)raw_frame->data, raw_frame->linesize, 0, dec_ctx->height, rgb_frame->data, rgb_frame->linesize);

				//save_frame(rgb_frame, 704, 576);
				{
					SDL_LockYUVOverlay(bmp);

					AVPicture pict;
					pict.data[0] = bmp->pixels[0];
					pict.data[1] = bmp->pixels[2];
					pict.data[2] = bmp->pixels[1];

					pict.linesize[0] = bmp->pitches[0];
					pict.linesize[1] = bmp->pitches[2];
					pict.linesize[2] = bmp->pitches[1];

					sws_scale(rgb2yuv, (const uint8_t * const*)rgb_frame->data, rgb_frame->linesize, 0, dec_ctx->height, pict.data, pict.linesize);

					 SDL_UnlockYUVOverlay(bmp);

					 SDL_Rect rect;
					 rect.x = 0;
					 rect.y = 0;
					 rect.w = dec_ctx->width;
					 rect.h = dec_ctx->height;
					 SDL_DisplayYUVOverlay(bmp, &rect);
				}


			}
			SDL_Event event;
			SDL_PollEvent(&event);
			if (event.type == SDL_QUIT)
			{
				SDL_Quit();
				goto end;
			}
		}
        }
	}

end:
	kill(res, SIGKILL);

	return 0;
}
