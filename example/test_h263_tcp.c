/*
 * test_h263_tcp.c
 *
 *  Created on: Sep 16, 2012
 *      Author: arash
 */

#define _POSIX_SOURCE

#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>

#include <tlog/tlog.h>

#include "rcpdefs.h"
#include "rcpplus.h"
#include "rcpcommand.h"
#include "rtp.h"
#include "coder.h"

int main()
{
	tlog_init(TLOG_MODE_STDERR, TLOG_INFO, NULL);

	rcp_connect("10.25.25.223");

	start_message_manager();

	client_register(RCP_USER_LEVEL_LIVE, "", RCP_REGISTRATION_TYPE_NORMAL, RCP_ENCRYPTION_MODE_MD5);

	rcp_coder_list encoders, decoders;
	get_coder_list(RCP_CODER_ENCODER, RCP_MEDIA_TYPE_VIDEO, &encoders, 1);
	TL_DEBUG("***");
	for (int i=0; i<encoders.count; i++)
		TL_DEBUG("%x %x %x %x %x", encoders.coder[i].number, encoders.coder[i].caps, encoders.coder[i].current_cap, encoders.coder[i].param_caps, encoders.coder[i].current_param);
	TL_DEBUG("***");
	get_coder_list(RCP_CODER_DECODER, RCP_MEDIA_TYPE_VIDEO, &decoders, 1);
	TL_DEBUG("***");
	for (int i=0; i<decoders.count; i++)
		TL_DEBUG("%x %x %x %x %x", decoders.coder[i].number, decoders.coder[i].caps, decoders.coder[i].current_cap, decoders.coder[i].param_caps, decoders.coder[i].current_param);
	TL_DEBUG("***");

	rcp_session session;
	memset(&session, 0, sizeof(rcp_session));
	unsigned short tcp_port = stream_connect_tcp(&session);

	rcp_media_descriptor desc = {
			RCP_MEP_TCP, 1, 1, 0, tcp_port, 0, 1, RCP_VIDEO_CODING_H263P, RCP_VIDEO_RESOLUTION_4CIF
	};

	client_connect(&session, RCP_CONNECTION_METHOD_GET, RCP_MEDIA_TYPE_VIDEO, 0, &desc);

	initiate_tcp_stream(&session, &decoders.coder[0]);

	int res = fork();
	if (res == 0)
	{
		while (1)
		{
			sleep(2);
			int n = keep_alive(&session);
			//TL_DEBUG("active connections = %d", n);
			if (n < 0)
				break;
		}
	}


	rtp_merge_desc mdesc;
	rtp_init(RTP_PAYLOAD_TYPE_H263, 1, &mdesc);

	time_t end_time = time(NULL) + 10;
	while (time(NULL) < end_time)
	{
/*
		int num = recv(con.stream_socket, buffer, 1500, 0);

		rtp_push_frame(buffer, num, &mdesc);
*/
		char buff[2000];
		int size = recv(session.stream_socket, buff, 1000, 0);
		fwrite(buff, size, 1, stdout);
/*
		rtp_recv(session.stream_socket, &mdesc);

		if (rtp_pop_frame(&mdesc) == 0)
			fwrite(mdesc.data, mdesc.frame_lenght, 1, stdout);
*/
	}

	stop_message_manager();


	return 0;
}
