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

	start_event_handler();

	client_register(RCP_USER_LEVEL_LIVE, "", RCP_REGISTRATION_TYPE_NORMAL, RCP_ENCRYPTION_MODE_MD5);

	get_coder_preset(1);

	preset_set_default(1);

	rcp_mpeg4_preset preset;
	get_preset(1, &preset, 0);
	log_preset(TLOG_INFO, &preset, 1);

	strcpy(preset.name, "myConfig");
	preset.resolution = PRESET_RESOLUTION_4CIF;
	preset.field_mode = PRESET_FIELD_MODE_PROGRESSIVE;
	preset.bandwidth = 100000;
	preset.bandwidth_soft_limit = 100000;
	preset.video_quality = 1;

	set_preset(1, &preset, 1);

	get_preset(1, &preset, 0);
	log_preset(TLOG_INFO, &preset, 1);

	client_unregister();

	stop_event_handler();

	return 0;
}
