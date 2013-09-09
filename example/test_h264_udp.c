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

#include "rcpdefs.h"
#include "rcpplus.h"
#include "rcpcommand.h"
#include "rtp.h"
#include "coder.h"

int main(int argc, char* argv[])
{
	tlog_init(TLOG_MODE_STDERR, TLOG_INFO, NULL);

    if (argc < 2)
    {
        INFO("%s ip", argv[0]);
        return 0;
    }

	rcp_connect(argv[1]);

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
	unsigned short udp_port = stream_connect_udp(&session);

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
	rtp_init(RTP_PAYLOAD_TYPE_H264, 1, &mdesc);
	video_frame vframe;

	while (1)
	{
/*
		int num = recvfrom(con.stream_socket, buffer, 1500, 0, (struct sockaddr*)&si_remote, &slen);

		rtp_push_frame(buffer, num, &mdesc);
*/
		rtp_recv(session.stream_socket, &mdesc);

		if (rtp_pop_frame(&vframe, &mdesc) == 0)
			fwrite(vframe.data, vframe.len, 1, stdout);
		//char cmd[100];
		//sprintf(cmd, "kill %d", res);
		//system(cmd);
		//return 0;
	}


	return 0;
}
