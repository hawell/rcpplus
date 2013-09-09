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
#include "libswscale/swscale.h"

#define VIDEO_MODE_H263	1
#define VIDEO_MODE_H264	2

int terminate = 0;

void handler(int n)
{
	INFO("SIGINT received");
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
	rtp_merge_desc mdesc;
	video_frame vframe;

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

	AVFormatContext* fc = NULL;

	char filename[] = "dump.mp4";
	enum CodecID codec_id = CODEC_ID_H264;
	int br = 1000000;
	int w = 704;
	int h = 576;
	int fps = 25;
	AVOutputFormat *of = av_guess_format(0, filename, 0);
	fc = avformat_alloc_context();
	fc->oformat = of;
	strcpy(fc->filename, filename);
	AVStream *pst = av_new_stream(fc, 0);
	AVCodecContext *pcc = pst->codec;
	avcodec_get_context_defaults2(pcc, AVMEDIA_TYPE_VIDEO);
	pcc->codec_type = AVMEDIA_TYPE_VIDEO;
	pcc->codec_id = codec_id;
	pcc->flags |= CODEC_FLAG_GLOBAL_HEADER;
	pcc->bit_rate = br;
	pcc->width = w;
	pcc->height = h;
	pcc->time_base.num = 1;
	pcc->time_base.den = fps;
	//av_set_parameters(fc, 0);
	if (!(fc->oformat->flags & AVFMT_NOFILE))
		avio_open(&fc->pb, fc->filename, AVIO_FLAG_WRITE);

	avformat_write_header(fc, NULL);

	//FILE* f = fopen("dump", "wb");

	signal(SIGINT, handler);
	while (!terminate)
	{
		if (rtp_recv(session.stream_socket, &mdesc) == 0)
        {
			if (rtp_pop_frame(&vframe, &mdesc) == 0)
			{
				//fwrite(vframe.data, vframe.len, 1, f);
				AVStream *pst = fc->streams[0];
				AVPacket pkt;
				av_init_packet(&pkt);
				pkt.flags |= (0 >= getVopType(vframe.data, vframe.len)) ? AV_PKT_FLAG_KEY : 0;
				pkt.stream_index = pst->index;
				pkt.data = (uint8_t*)vframe.data;
				pkt.size = vframe.len;

				pkt.dts = AV_NOPTS_VALUE;
				pkt.pts = AV_NOPTS_VALUE;

				av_interleaved_write_frame(fc, &pkt);
			}
        }
	}

	//fclose(f);
	INFO("writing trailer");
	av_write_trailer(fc);
	if (fc->oformat && !( fc->oformat->flags & AVFMT_NOFILE ) && fc->pb)
		avio_close( fc->pb );
	av_free(fc);
	kill(res, SIGKILL);

	return 0;
}
