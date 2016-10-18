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

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        printf("%s <ip>\n", argv[0]);
        return 0;
    }

	tlog_init(TLOG_MODE_STDERR, TLOG_INFO, NULL);

	rcp_connect(argv[1]);

	start_message_manager();

	client_register(RCP_USER_LEVEL_LIVE, "", RCP_REGISTRATION_TYPE_NORMAL, RCP_ENCRYPTION_MODE_MD5);

	int preset_id = get_coder_preset(1);
	TL_INFO("preset id = %d", preset_id);
	preset_id = get_current_stream_profile(1, 1, &preset_id);
	TL_INFO("preset id = %d", preset_id);

	preset_set_default(1);

	rcp_mpeg4_preset preset;
    get_preset(1, &preset, 0);
    log_preset(TLOG_INFO, &preset, 0);

	strcpy(preset.name, "myConfig");
	preset.resolution = PRESET_RESOLUTION_4CIF;
	preset.field_mode = PRESET_FIELD_MODE_PROGRESSIVE;
	preset.bandwidth = 100000;
	preset.bandwidth_soft_limit = 100000;
	preset.video_quality = 1;
    preset.avc_gop_structure = 0;
    preset.averaging_period = 0;
    preset.iframe_distance = 31;
    preset.avc_pframe_qunatizer_min = 0;
    preset.avc_delta_ipquantizer = 5;

	set_preset(1, &preset, 0);

	get_preset(1, &preset, 0);
	log_preset(TLOG_INFO, &preset, 0);

	client_unregister();

	stop_message_manager();

	return 0;
}
