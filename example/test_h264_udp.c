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

	rcp_connect("174.0.0.224");

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

	struct sockaddr_in si_remote;
	socklen_t slen = sizeof(si_remote);
	unsigned char buffer[1500];

	nal_packet np[2];
	np[0].size = 0;
	np[1].size = 0;
	while (1)
	{
		int num = recvfrom(con.stream_socket, buffer, 1500, 0, (struct sockaddr*)&si_remote, &slen);

		int count = defrag(np, buffer, num);
		for (int i=0; i<count; i++)
		{
			fwrite(NAL_START_FRAME, NAL_START_FRAME_LENGTH, 1, stdout);
			fwrite(&np[i].nh, 1, 1, stdout);
			fwrite(np[i].payload, np[i].size, 1, stdout);
			np[i].size = 0;
		}
		//char cmd[100];
		//sprintf(cmd, "kill %d", res);
		//system(cmd);
		//return 0;
	}


	return 0;
}
