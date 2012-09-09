/*
 * test.c
 *
 *  Created on: Aug 22, 2012
 *      Author: arash
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "rcpdefs.h"
#include "rcpplus.h"
#include "rcpcommand.h"
#include "rcplog.h"
#include "preset.h"

int main()
{
	rcplog_init(LOG_MODE_STDERR, LOG_INFO, NULL);

	rcp_connect("174.0.0.236");

	rcp_session session;
	memset(&session, 0, sizeof(rcp_session));
	session.user_level = RCP_USER_LEVEL_LIVE;

	client_register(RCP_REGISTRATION_TYPE_NORMAL, RCP_ENCRYPTION_MODE_MD5, &session);

	get_coder_preset(&session, 1);

	preset_set_default(&session, 1);

	rcp_mpeg4_preset preset;
	get_preset(&session, 1, &preset, 0);
	log_preset(LOG_INFO, &preset, 1);

	strcpy(preset.name, "myConfig");
	preset.resolution = PRESET_RESOLUTION_4CIF;
	preset.bandwidth = 10000;
	preset.bandwidth_soft_limit = 10000;

	set_preset(&session, 1, &preset, 1);

	get_preset(&session, 1, &preset, 0);
	log_preset(LOG_INFO, &preset, 1);

	return 0;
}
