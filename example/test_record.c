/*
 * test_record.c
 *
 *  Created on: Jul 13, 2013
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
#include <pthread.h>

#include "rcpdefs.h"
#include "rcpplus.h"
#include "rcpcommand.h"
#include "rtp.h"
#include "coder.h"
#include "preset.h"

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavformat/avio.h"
#include "libavutil/avutil.h"
#include "libavutil/imgutils.h"

#define VIDEO_MODE_H263	1
#define VIDEO_MODE_H264	2

int terminate = 0;

void handler(int n)
{
	UNUSED(n);

	TL_INFO("SIGINT received");
	signal(SIGINT, SIG_IGN);
	terminate = 1;
}

// < 0 = error
// 0 = I-Frame
// 1 = P-Frame
// 2 = B-Frame
// 3 = S-Frame
int getVopType( const void *p, int len )
{
    if ( !p || 6 >= len )
        return -1;

    unsigned char *b = (unsigned char*)p;

    // Verify NAL marker
    if ( b[ 0 ] || b[ 1 ] || 0x01 != b[ 2 ] )
    {   b++;
        if ( b[ 0 ] || b[ 1 ] || 0x01 != b[ 2 ] )
            return -1;
    } // end if

    b += 3;

    // Verify VOP id
    if ( 0xb6 == *b )
    {   b++;
        return ( *b & 0xc0 ) >> 6;
    } // end if

    switch( *b )
    {   case 0x65 : return 0;
        case 0x61 : return 1;
        case 0x01 : return 2;
    } // end switch

    return -1;
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

unsigned int highest_bit(unsigned int a)
{
	if (!a)
		return 0;

	unsigned int r = 1;
	while (a>>=1)
		r <<= 1;

	return r;
}

int main(int argc, char* argv[])
{
	tlog_init(TLOG_MODE_STDERR, TLOG_INFO, NULL);

	if (argc < 2)
	{
		TL_INFO("%s ip\n", argv[0]);
		return 0;
	}

	rcp_connect(argv[1]);

	start_message_manager();

	client_register(RCP_USER_LEVEL_LIVE, "", RCP_REGISTRATION_TYPE_NORMAL, RCP_ENCRYPTION_MODE_MD5);

	rcp_coder_list encoders;
	get_coder_list(RCP_CODER_DECODER, RCP_MEDIA_TYPE_VIDEO, &encoders, 1);
	int coder_id = 1;
	int resolution = RCP_VIDEO_RESOLUTION_QCIF;
	for (int i=0; i<encoders.count; i++)
	{
		if ((encoders.coder[i].current_cap & RCP_VIDEO_CODING_H264) || (encoders.coder[i].current_cap & RCP_VIDEO_CODING_H263))
		{
			log_coder(TLOG_INFO, &encoders.coder[i]);
			coder_id = encoders.coder[i].number;
			resolution = encoders.coder[i].current_param;
			resolution &= 0x1EF;
			break;
		}
	}

	resolution = highest_bit(resolution);

	int width, height;
	switch (resolution)
	{
		case RCP_VIDEO_RESOLUTION_HD1080:
			width = 1920;
			height = 1080;
		break;

		case RCP_VIDEO_RESOLUTION_HD720:
			width = 1280;
			height = 720;
		break;

		case RCP_VIDEO_RESOLUTION_VGA:
			width = 640;
			height = 480;
		break;

		case RCP_VIDEO_RESOLUTION_QVGA:
			width = 320;
			height = 240;
		break;

		case RCP_VIDEO_RESOLUTION_4CIF:
			width = 704;
			height = 576;
		break;

		case RCP_VIDEO_RESOLUTION_2CIF:
			width = 704;
			height = 288;
		break;

		case RCP_VIDEO_RESOLUTION_CIF:
			width = 352;
			height = 288;
		break;

		case RCP_VIDEO_RESOLUTION_QCIF:
			width = 176;
			height = 144;
		break;

	}

	int preset_id = get_coder_preset(coder_id);
	rcp_mpeg4_preset preset;
	get_preset(preset_id, &preset, 1);

	int video_mode;
	get_coder_video_operation_mode(coder_id, &video_mode);

	TL_INFO("mode=%d res=%d id=%d", video_mode, resolution, coder_id);

	int coding;
	if (video_mode == VIDEO_MODE_H263)
		coding = RCP_VIDEO_CODING_MPEG4;
	else
		coding = RCP_VIDEO_CODING_H264;

	//avcodec_register_all();
	//av_register_all();
	rtp_merge_desc mdesc;

	if (video_mode == VIDEO_MODE_H263)
	{
		rtp_init(RTP_PAYLOAD_TYPE_H263, 1, &mdesc);
	}
	else
	{
		rtp_init(RTP_PAYLOAD_TYPE_H264, 1, &mdesc);
	}

	AVPacket in_pkt;
	av_init_packet(&in_pkt);

	rcp_session session;
	memset(&session, 0, sizeof(rcp_session));

	unsigned short udp_port = stream_connect_udp(&session);
	rcp_media_descriptor desc = {
			RCP_MEP_UDP, 1, 1, 0, udp_port, coder_id, 1, coding, resolution
	};

	client_connect(&session, RCP_CONNECTION_METHOD_GET, RCP_MEDIA_TYPE_VIDEO, 0, &desc);

	pthread_create(&thread, NULL, keep_alive_thread, &session);

	AVFormatContext* fc = NULL;
	const AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_H264);
	char filename[] = "dump.mp4";
	enum AVCodecID codec_id = AV_CODEC_ID_H264;
	int br = 1000000;
	const AVOutputFormat *of = av_guess_format(0, filename, 0);
	fc = avformat_alloc_context();
	fc->oformat = of;
	//strcpy(fc->filename, filename);
	AVStream *pst = avformat_new_stream(fc, codec);
	pst->index = 0;
	AVCodecContext *pcc = avcodec_alloc_context3(codec);
	AVCodecParameters *params = pst->codecpar;
	avcodec_parameters_to_context(pcc, params);

	pcc->codec_type = AVMEDIA_TYPE_VIDEO;
	pcc->codec_id = codec_id;
	pcc->bit_rate = br;
	pcc->width = width;
	pcc->height = height;
	pcc->time_base.num = 1;
	pcc->time_base.den = 1000;
	pcc->pix_fmt = AV_PIX_FMT_YUV420P;
	if (fc->oformat->flags & AVFMT_GLOBALHEADER)
		pcc->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

	if (avcodec_open2(pcc, codec, NULL) < 0)
	{
		TL_INFO("unable to open codec");
		exit(1);
	}

	if (!(fc->oformat->flags & AVFMT_NOFILE))
		avio_open(&fc->pb, filename, AVIO_FLAG_WRITE);

	int rc;
	if ((rc = avformat_write_header(fc, NULL)) < 0) {
		TL_INFO("avformat_write_header(): %s", av_err2str(rc));
	};

	//FILE* f = fopen("dump", "wb");

	signal(SIGINT, handler);
	struct timeval start_time;
	gettimeofday(&start_time, NULL);

/*
    AVFrame* frame = avcodec_alloc_frame();
    if (!frame) {
        fprintf(stderr, "Could not allocate video frame\n");
        exit(1);
    }
    frame->format = pcc->pix_fmt;
    frame->width  = pcc->width;
    frame->height = pcc->height;

    av_image_alloc(frame->data, frame->linesize, pcc->width, pcc->height, pcc->pix_fmt, 32);

    {
    	TL_INFO("internal: %p", pcc->internal);
		int got_output = 0;
		AVPacket pkt;
		av_init_packet(&pkt);
		pkt.data = NULL;
		pkt.size = 0;
		pkt.flags |= AV_PKT_FLAG_KEY;
		if (avcodec_encode_video2(pcc, &pkt, frame, &got_output) && got_output)
		{
			pkt.dts = pkt.pts = AV_NOPTS_VALUE;
			pkt.stream_index = pst->index;
			av_write_frame(fc, &pkt);
		}
    }

*/
	while (!terminate)
	{
		if (rtp_recv(session.stream_socket, &mdesc) == 0)
        {
			if (rtp_pop_frame(&mdesc) == 0)
			{
				//fwrite(vframe.data, vframe.len, 1, f);
				AVStream *pst = fc->streams[0];
				AVPacket pkt;
				av_init_packet(&pkt);
				pkt.flags |= (0 >= getVopType(mdesc.data, mdesc.frame_lenght)) ? AV_PKT_FLAG_KEY : 0;
				pkt.stream_index = pst->index;
				pkt.data = (uint8_t*)mdesc.data;
				pkt.size = mdesc.frame_lenght;

				pkt.dts = AV_NOPTS_VALUE;
				struct timeval recv_time;
				gettimeofday(&recv_time, NULL);
				pkt.pts = av_rescale_q((recv_time.tv_sec-start_time.tv_sec)*1000 + (recv_time.tv_usec-start_time.tv_usec)/1000, pcc->time_base, pst->time_base);

				av_write_frame(fc, &pkt);
			}
        }
	}

	//fclose(f);
	TL_INFO("writing trailer");
	av_write_trailer(fc);
	if (fc->oformat && !( fc->oformat->flags & AVFMT_NOFILE ) && fc->pb)
		avio_close( fc->pb );
	av_free(fc);

	pthread_cancel(thread);

	client_disconnect(&session);

	client_unregister();

	stop_message_manager();

	return 0;
}
