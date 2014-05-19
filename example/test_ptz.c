/*
 * test_ptz.c
 *
 *  Created on: Sep 15, 2012
 *      Author: arash
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <tlog/tlog.h>

#include "rcpdefs.h"
#include "rcpplus.h"
#include "rcpcommand.h"
#include "ptz.h"

int main(int argc, char* argv[])
{
	if (argc < 2)
	{
		fprintf(stderr, "%s <ip>", argv[0]);
		return 0;
	}
	tlog_init(TLOG_MODE_STDERR, TLOG_INFO, NULL);

	rcp_connect(argv[1]);

	start_event_handler();

	client_register(RCP_USER_LEVEL_LIVE, "", RCP_REGISTRATION_TYPE_NORMAL, RCP_ENCRYPTION_MODE_MD5);

	if (ptz_available() == 0)
	{
		TL_INFO("ptz is not available for this camera");
		//return 0;
	}

	move_right(0, 10, 5);
	sleep(1);
	move_left(0, 10, 5);
	sleep(1);
	move_up(0, 10, 5);
	sleep(1);
	move_down(0, 10, 5);
	sleep(1);
	move_up_right(0, 10, 5, 5);
	sleep(1);
	move_up_left(0, 10, 5, 5);
	sleep(1);
	move_down_right(0, 10, 5, 5);
	sleep(1);
	move_down_left(0, 10, 5, 5);
	sleep(1);
	move_stop(0, 10);

	zoom_in(0, 10, 5);
	sleep(1);
	zoom_out(0, 10, 5);
	sleep(1);
	move_stop(0, 10);

	preposition_set(0, 10, 3);
	sleep(1);
	iris_brighter(0, 10);
	sleep(1);
	move_right(0, 10, 5);
	sleep(2);
	move_stop(0, 10);
	sleep(1);
	preposition_shot(0, 10, 3);
	sleep(1);

	stop_event_handler();

	return 0;
}
