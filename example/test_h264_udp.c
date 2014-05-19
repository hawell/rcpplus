/*
 * test_h264_udp.c
 *
 *  Created on: Aug 22, 2012
 *      Author: arash
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <tlog/tlog.h>
#include <pthread.h>
#include <signal.h>

#include "rcpdefs.h"
#include "rcpplus.h"
#include "rcpcommand.h"
#include "rtp.h"
#include "coder.h"

int end = 0;

void term_handler(int x __attribute__((unused)))
{
	end = 1;
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

    if (argc < 3)
    {
        TL_INFO("%s <ip> <output>", argv[0]);
        return 0;
    }

	rcp_connect(argv[1]);

	start_event_handler();

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
	unsigned short udp_port = stream_connect_udp(&session);

	TL_DEBUG("udp port = %d", udp_port);

	rcp_media_descriptor desc = {
			RCP_MEP_UDP, 1, 1, 0, udp_port, 1, 1, RCP_VIDEO_CODING_H264, RCP_VIDEO_RESOLUTION_4CIF
	};

	client_connect(&session, RCP_CONNECTION_METHOD_GET, RCP_MEDIA_TYPE_VIDEO, 0, &desc);

	pthread_create(&thread, NULL, keep_alive_thread, &session);

	rtp_merge_desc mdesc;
	rtp_init(RTP_PAYLOAD_TYPE_H264, 1, &mdesc);
	video_frame vframe;

	signal(SIGTERM, term_handler);

	FILE * out = fopen(argv[2], "wb");

	while (!end)
	{
/*
		int num = recvfrom(con.stream_socket, buffer, 1500, 0, (struct sockaddr*)&si_remote, &slen);

		rtp_push_frame(buffer, num, &mdesc);
*/
		if (rtp_recv(session.stream_socket, &mdesc) == 0)
        {
			if (rtp_pop_frame(&vframe, &mdesc) == 0)
			{
				fwrite(vframe.data, vframe.len, 1, out);
			}
        }
		//char cmd[100];
		//sprintf(cmd, "kill %d", res);
		//system(cmd);
		//return 0;
	}

	fclose(out);

	pthread_cancel(thread);

	client_disconnect(&session);

	client_unregister();

	stop_event_handler();

	return 0;
}
