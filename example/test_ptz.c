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

int main()
{
	tlog_init(TLOG_MODE_STDERR, TLOG_DEBUG, NULL);

	rcp_connect("10.25.25.220");

	client_register(RCP_USER_LEVEL_LIVE, "", RCP_REGISTRATION_TYPE_NORMAL, RCP_ENCRYPTION_MODE_MD5);

	if (ptz_available() == 0)
	{
		INFO("ptz is not available for this camera");
		return 0;
	}

	move_right(5);
	sleep(1);
	move_left(5);
	sleep(1);
	move_up(5);
	sleep(1);
	move_down(5);
	sleep(1);
	move_stop();

	zoom_in(5);
	sleep(1);
	zoom_out(5);
	sleep(1);
	move_stop();

	return 0;
}
