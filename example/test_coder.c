/*
 * test_coder.c
 *
 *  Created on: Sep 10, 2012
 *      Author: arash
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <tlog/tlog.h>

#include "rcpdefs.h"
#include "rcpplus.h"
#include "rcpcommand.h"
#include "coder.h"


int main(int argc, char* argv[])
{
	tlog_init(TLOG_MODE_STDERR, TLOG_INFO, NULL);

	if (argc < 2)
	{
		TL_INFO("%s ip\n", argv[0]);
		return 0;
	}

	rcp_connect(argv[1]);

	start_event_handler();

	client_register(RCP_USER_LEVEL_LIVE, "", RCP_REGISTRATION_TYPE_NORMAL, RCP_ENCRYPTION_MODE_MD5);

	rcp_coder_list encoders;
	get_coder_list(RCP_CODER_DECODER, RCP_MEDIA_TYPE_VIDEO, &encoders);
	for (int i=0; i<encoders.count; i++)
	{
		int mode;
		get_coder_video_operation_mode(encoders.coder[i].number, &mode);
		TL_INFO("video mode is %d", mode);
		TL_INFO("%x %x %x %x %x", encoders.coder[i].number, encoders.coder[i].caps, encoders.coder[i].current_cap, encoders.coder[i].param_caps, encoders.coder[i].current_param);
		log_coder(TLOG_INFO, &encoders.coder[i]);

		mode = (mode==1)?2:1;
		mode = set_coder_video_operation_mode(encoders.coder[i].number, mode);
		TL_INFO("video mode is %d", mode);
		TL_INFO("-----------------------");
	}


	get_coder_list(RCP_CODER_DECODER, RCP_MEDIA_TYPE_AUDIO, &encoders);
	for (int i=0; i<encoders.count; i++)
	{
		TL_INFO("%x %x %x %x %x", encoders.coder[i].number, encoders.coder[i].caps, encoders.coder[i].current_cap, encoders.coder[i].param_caps, encoders.coder[i].current_param);
		log_coder(TLOG_INFO, &encoders.coder[i]);
		TL_INFO("-----------------------");
	}

	stop_event_handler();

	return 0;
}
