/*
 * test.c
 *
 *  Created on: Aug 22, 2012
 *      Author: arash
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <tlog/tlog.h>

#include "rcpdefs.h"
#include "rcpplus.h"
#include "rcpcommand.h"
#include "preset.h"
#include "coder.h"

int main()
{
	tlog_init(TLOG_MODE_STDERR, TLOG_INFO, NULL);

	rcp_connect("10.25.25.220");

	rcp_session session;
	memset(&session, 0, sizeof(rcp_session));
	session.user_level = RCP_USER_LEVEL_LIVE;

	client_register(RCP_REGISTRATION_TYPE_NORMAL, RCP_ENCRYPTION_MODE_MD5, &session);

	get_coder_preset(&session, 1);

	preset_set_default(&session, 1);

	rcp_mpeg4_preset preset;
	get_preset(&session, 1, &preset, 0);
	log_preset(TLOG_INFO, &preset, 1);

	strcpy(preset.name, "myConfig");
	preset.resolution = PRESET_RESOLUTION_4CIF;
	preset.field_mode = PRESET_FIELD_MODE_PROGRESSIVE;
	preset.bandwidth = 100000;
	preset.bandwidth_soft_limit = 100000;
	preset.video_quality = 1;

	set_preset(&session, 1, &preset, 1);

	get_preset(&session, 1, &preset, 0);
	log_preset(TLOG_INFO, &preset, 1);

	return 0;
}
