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

#include "rcpdefs.h"
#include "rcpplus.h"
#include "rcpcommand.h"
#include "rtp.h"
#include "rcplog.h"
#include "coder.h"

int main()
{
	rcplog_init(LOG_MODE_STDERR, LOG_INFO, NULL);

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
			RCP_MEP_UDP, 1, 1, 0, udp_port, 1, 1, RCP_VIDEO_CODING_H264, RCP_VIDEO_RESOLUTION_4CIF
	};

	client_connect(&session, RCP_CONNECTION_METHOD_GET, RCP_MEDIA_TYPE_VIDEO, 0, &desc);

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

	rtp_merge_desc mdesc;
	rtp_init(RTP_PAYLOAD_TYPE_H264, &mdesc);
	video_frame vframe;

	while (1)
	{
/*
		int num = recvfrom(con.stream_socket, buffer, 1500, 0, (struct sockaddr*)&si_remote, &slen);

		rtp_push_frame(buffer, num, &mdesc);
*/
		rtp_recv(con.stream_socket, &mdesc);

		if (rtp_pop_frame(&vframe, &mdesc) == 0)
			fwrite(vframe.data, vframe.len, 1, stdout);
		//char cmd[100];
		//sprintf(cmd, "kill %d", res);
		//system(cmd);
		//return 0;
	}


	return 0;
}
