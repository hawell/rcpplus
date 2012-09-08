/*
 * test_mpeg4.c
 *
 *  Created on: Sep 1, 2012
 *      Author: arash
 */

/*
 * test.c
 *
 *  Created on: Aug 22, 2012
 *      Author: arash
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>

#include "rcpdefs.h"
#include "rcpplus.h"
#include "rcpcommand.h"
#include "rtp.h"
#include "rcplog.h"

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>

#define MAX_FRAME_LEN	100000

unsigned char sbit_mask[] = {0xFF, 0x7F, 0x3F, 0x1F, 0x0F, 0x07, 0x03, 0x01, 0x00};
unsigned char ebit_mask[] = {0xFF, 0xFE, 0xFC, 0xF8, 0xF0, 0xE0, 0xC0, 0x80, 0x00};

int main()
{
	rcplog_init(LOG_MODE_STDERR, LOG_INFO, NULL);

	rcp_connect("174.0.0.236");

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

	unsigned short udp_port = stream_connect();

	DEBUG("udp port = %d", udp_port);

	rcp_media_descriptor desc[] = {
			{RCP_MEP_UDP, 1, 1, 0, udp_port, 1, 1, RCP_VIDEO_CODING_MPEG4, RCP_VIDEO_RESOLUTION_2CIF},
			//{RCP_MEP_UDP, 1, 1, 0, udp_port, 1, 1, RCP_VIDEO_CODING_JPEG, RCP_VIDEO_RESOLUTION_4CIF},
			{0}
	};

	client_connect(&session, RCP_CONNECTION_METHOD_GET, RCP_MEDIA_TYPE_VIDEO, 0, desc);

	int res = fork();
	if (res == 0)
	{
		while (1)
		{
			int n = keep_alive(&session);
			//DEBUG("active connections = %d", n);
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
	int num = 1000;

	unsigned short last_seq = 0;
	int ebit = 0;
	while (1)
	{
		int num = recvfrom(con.stream_socket, buffer, 1500, 0, (struct sockaddr*)&si_remote, &slen);

		rtp_header *hdr = buffer;
		int header_len = RTP_HEADER_LENGTH_MANDATORY + hdr->cc*4;
		DEBUG("m=%d pt=%d hlen=%d len=%d seq=%d", hdr->m, hdr->pt, header_len, num, hdr->seq);

		last_seq = hdr->seq;

		h263_header *hdr2 = buffer+header_len;
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
			DEBUG("writing %d bytes", complete_frame_size);
			int ret = fwrite(complete_frame, complete_frame_size, 1, stdout);
			DEBUG("written %d", ret);
			complete_frame_size = 0;
		}
	}


	return 0;
}
