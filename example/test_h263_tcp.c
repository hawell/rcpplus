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

	rcp_connect("10.25.25.220");

	start_event_handler();

	client_register(RCP_USER_LEVEL_LIVE, "", RCP_REGISTRATION_TYPE_NORMAL, RCP_ENCRYPTION_MODE_MD5);

	get_capability_list();

	rcp_coder_list encoders, decoders;
	get_coder_list(RCP_CODER_ENCODER, RCP_MEDIA_TYPE_VIDEO, &encoders);
	DEBUG("***");
	for (int i=0; i<encoders.count; i++)
		DEBUG("%x %x %x %x %x", encoders.coder[i].number, encoders.coder[i].caps, encoders.coder[i].current_cap, encoders.coder[i].param_caps, encoders.coder[i].current_param);
	DEBUG("***");
	get_coder_list(RCP_CODER_DECODER, RCP_MEDIA_TYPE_VIDEO, &decoders);
	DEBUG("***");
	for (int i=0; i<decoders.count; i++)
		DEBUG("%x %x %x %x %x", decoders.coder[i].number, decoders.coder[i].caps, decoders.coder[i].current_cap, decoders.coder[i].param_caps, decoders.coder[i].current_param);
	DEBUG("***");

	rcp_session session;
	memset(&session, 0, sizeof(rcp_session));
	unsigned short udp_port = stream_connect_tcp(&session);

	ERROR("port = %d", udp_port);

	rcp_media_descriptor desc = {
			RCP_MEP_TCP, 1, 1, 0, udp_port, 1, 1, RCP_VIDEO_CODING_MPEG4, RCP_VIDEO_RESOLUTION_4CIF
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
			//DEBUG("active connections = %d", n);
			if (n < 0)
				break;
		}
	}

	rtp_merge_desc mdesc;
	rtp_init(RTP_PAYLOAD_TYPE_H263, 1, &mdesc);
	video_frame vframe;

	time_t end_time = time(NULL) + 10;
	while (time(NULL) < end_time)
	{
/*
		int num = recv(con.stream_socket, buffer, 1500, 0);

		rtp_push_frame(buffer, num, &mdesc);
*/
		rtp_recv(session.stream_socket, &mdesc);

		if (rtp_pop_frame(&vframe, &mdesc) == 0)
			fwrite(vframe.data, vframe.len, 1, stdout);
	}

	stop_event_handler();


	return 0;
}
