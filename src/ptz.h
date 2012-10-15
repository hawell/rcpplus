/*
 * ptz.h
 *
 *  Created on: Sep 15, 2012
 *      Author: arash
 *
 *  This file is part of rcpplus
 *
 *  rcpplus is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  rcpplus is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with rcpplus.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef PTZ_H_
#define PTZ_H_

#include "rcpplus.h"

#define RCP_COMMAND_CONF_RCP_TRANSFER_TRANSPARENT_DATA	0xffdd
#define RCP_COMMAND_CONF_PTZ_CONTROLLER_AVAILABLE	0x0a51

typedef struct {
	unsigned char tilt_speed:4;
	unsigned char zoom_speed:3;
	unsigned char db1_res:1;

	unsigned char focus_far:1;
	unsigned char iris_darker:1;
	unsigned char iris_brighter:1;
	unsigned char pan_speed:4;
	unsigned char db2_res:1;

	unsigned char pan_right:1;
	unsigned char pan_left:1;
	unsigned char tilt_down:1;
	unsigned char tilt_up:1;
	unsigned char zoom_out:1;
	unsigned char zoom_in:1;
	unsigned char focus_near:1;
	unsigned char db3_res:1;
} VarSpeedPTZ;

int ptz_available(rcp_session* session);

int move_stop(rcp_session* session);

int move_right(rcp_session* session, int speed);
int move_left(rcp_session* session, int speed);

int move_up(rcp_session* session, int speed);
int move_down(rcp_session* session, int speed);

int zoom_in(rcp_session* session, int speed);
int zoom_out(rcp_session* session, int speed);

int focus_far(rcp_session* session);
int focus_near(rcp_session* session);

int iris_darker(rcp_session* session);
int iris_brighter(rcp_session* session);

#endif /* PTZ_H_ */
