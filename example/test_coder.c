/*
 * test_coder.c
 *
 *  Created on: Sep 10, 2012
 *      Author: arash
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "rcpdefs.h"
#include "rcpplus.h"
#include "rcpcommand.h"
#include "rcplog.h"
#include "coder.h"


int main()
{
	rcplog_init(LOG_MODE_STDERR, LOG_INFO, NULL);

	rcp_connect("174.0.0.236");

	rcp_session session;
	memset(&session, 0, sizeof(rcp_session));
	session.user_level = RCP_USER_LEVEL_LIVE;

	client_register(RCP_REGISTRATION_TYPE_NORMAL, RCP_ENCRYPTION_MODE_MD5, &session);

	rcp_coder_list encoders;
	get_coder_list(&session, RCP_CODER_DECODER, RCP_MEDIA_TYPE_VIDEO, &encoders);
	for (int i=0; i<encoders.count; i++)
	{
		int mode;
		get_coder_video_operation_mode(&session, encoders.coder[i].number, &mode);
		INFO("video mode is %d", mode);
		INFO("%x %x %x %x %x", encoders.coder[i].number, encoders.coder[i].caps, encoders.coder[i].current_cap, encoders.coder[i].param_caps, encoders.coder[i].current_param);
		log_coder(LOG_INFO, &encoders.coder[i]);

		mode = (mode==1)?2:1;
		mode = set_coder_video_operation_mode(&session, encoders.coder[i].number, mode);
		INFO("video mode is %d", mode);
		INFO("-----------------------");
	}

	return 0;
}
