/*
 * test_h263_udp.c
 *
 *  Created on: Sep 1, 2012
 *      Author: arash
 */

#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <signal.h>
#include <time.h>
#include <tlog/tlog.h>
#include <pthread.h>

#include "rcpdefs.h"
#include "rcpplus.h"
#include "rcpcommand.h"
#include "rtp.h"
#include "coder.h"

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

int main()
{
	tlog_init(TLOG_MODE_STDERR, TLOG_INFO, NULL);

	rcp_connect("10.25.25.220");

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
			RCP_MEP_UDP, 1, 1, 0, udp_port, 0, 1, RCP_VIDEO_CODING_MPEG4, RCP_VIDEO_RESOLUTION_4CIF
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
			fwrite(vframe.data, vframe.len, 1, stdout);
	}

	pthread_cancel(thread);

	stop_event_handler();

	return 0;
}
