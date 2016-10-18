/*
 * test_alarm.c
 *
 *  Created on: Nov 24, 2015
 *      Author: arash
 */

#include <stddef.h>
#include <unistd.h>

#include <tlog/tlog.h>

#include "rcpdefs.h"
#include "rcpplus.h"
#include "rcpcommand.h"
#include "coder.h"
#include "audio.h"
#include "alarm.h"

void alarm_handler(rcp_packet* data, void* param)
{
	TL_INFO("alarm");
	UNUSED(param);
	tlog_hex(TLOG_INFO, "payload : ", data->payload, data->payload_length);
}

int main(int argc, char *argv[])
{
	tlog_init(TLOG_MODE_STDERR, TLOG_INFO, NULL);

	if (argc < 2)
	{
		TL_INFO("%s <ip>\n", argv[0]);
		return 0;
	}

	rcp_connect(argv[1]);

	start_message_manager();

	client_register(RCP_USER_LEVEL_LIVE, "", RCP_REGISTRATION_TYPE_NORMAL, RCP_ENCRYPTION_MODE_MD5);


	unsigned int id = get_viproc_id(1);
	TL_INFO("viproc id: %u", id);
	TL_INFO("viproc mode: %d", get_viproc_mode(1));
	TL_INFO("vca mode: %d", get_vca_status(1));

	register_event(RCP_COMMAND_CONF_VIPROC_ALARM, alarm_handler, NULL);

	get_viproc_alarm_mask(1);

	image_grid grid;

	viproc_start_edit(1, id);
	get_viproc_sensitive_area(1, &grid);
	viproc_stop_edit(1, id);

	while (1)
	{
		sleep(1);
	}

	return 0;
}
