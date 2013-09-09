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


int main()
{
	tlog_init(TLOG_MODE_STDERR, TLOG_INFO, NULL);

	rcp_connect("10.25.25.220");

	client_register(RCP_USER_LEVEL_LIVE, "", RCP_REGISTRATION_TYPE_NORMAL, RCP_ENCRYPTION_MODE_MD5);

	rcp_coder_list encoders;
	get_coder_list(RCP_CODER_DECODER, RCP_MEDIA_TYPE_VIDEO, &encoders);
	for (int i=0; i<encoders.count; i++)
	{
		int mode;
		get_coder_video_operation_mode(encoders.coder[i].number, &mode);
		INFO("video mode is %d", mode);
		INFO("%x %x %x %x %x", encoders.coder[i].number, encoders.coder[i].caps, encoders.coder[i].current_cap, encoders.coder[i].param_caps, encoders.coder[i].current_param);
		log_coder(TLOG_INFO, &encoders.coder[i]);

		mode = (mode==1)?2:1;
		mode = set_coder_video_operation_mode(encoders.coder[i].number, mode);
		INFO("video mode is %d", mode);
		INFO("-----------------------");
	}

	return 0;
}
