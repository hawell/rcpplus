/*
 * test_ptz.c
 *
 *  Created on: Sep 15, 2012
 *      Author: arash
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "rcpdefs.h"
#include "rcpplus.h"
#include "rcpcommand.h"
#include "rcplog.h"
#include "ptz.h"

int main()
{
	rcplog_init(LOG_MODE_STDERR, RCP_LOG_DEBUG, NULL);

	rcp_connect("174.0.0.236");

	rcp_session session;
	memset(&session, 0, sizeof(rcp_session));
	session.user_level = RCP_USER_LEVEL_LIVE;

	client_register(RCP_REGISTRATION_TYPE_NORMAL, RCP_ENCRYPTION_MODE_MD5, &session);

	if (ptz_available(&session) == 0)
	{
		INFO("ptz is not available for this camera");
		return 0;
	}

	move_right(&session, 5);
	sleep(1);
	move_left(&session, 5);
	sleep(1);
	move_up(&session, 5);
	sleep(1);
	move_down(&session, 5);
	sleep(1);
	move_stop(&session);

	zoom_in(&session, 5);
	sleep(1);
	zoom_out(&session, 5);
	sleep(1);
	move_stop(&session);

	return 0;
}
