/*
 * ptz.c
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

#include <stdlib.h>
#include <string.h>

#include "ptz.h"
#include "rcpdefs.h"
#include "rcplog.h"

typedef struct {
	unsigned char option;
	unsigned char reserved;
	unsigned short lease_time;
	unsigned char packet_len;
	unsigned short address;
	unsigned char opcode;
	unsigned char data_byte[3];
} osrd_packet;

static unsigned char checksum(unsigned char* data, int len)
{
	unsigned char sum = 0;
	for (int i=0; i<len; i++)
		sum += data[i];
	return (sum & 0x7f);
}

static int send_osrd(rcp_session* session, int lease_time, int opcode, unsigned char* data, int data_len)
{
	rcp_packet osrd_req;
	int res;

	init_rcp_header(&osrd_req, session, RCP_COMMAND_CONF_RCP_TRANSFER_TRANSPARENT_DATA, RCP_COMMAND_MODE_WRITE, RCP_DATA_TYPE_P_OCTET);
	osrd_req.numeric_descriptor = 1;

	unsigned short tmp16;
	osrd_req.payload[0] = 0; // options : Currently no options used
	osrd_req.payload[1] = 0; // reserved
	tmp16 = htons(lease_time);
	memcpy(osrd_req.payload+2, &tmp16, 2);

	osrd_req.payload[4] = 0x80 + 4 + data_len; // number of bytes in the remainder of the packet

	osrd_req.payload[5] = 0;
	osrd_req.payload[6] = 0; // address

	osrd_req.payload[7] = opcode;

	memcpy(osrd_req.payload+8, data, data_len);

	osrd_req.payload[8+data_len] = checksum(osrd_req.payload+4, data_len+4);

	osrd_req.payload_length = 8 + data_len + 1;

	res = rcp_send(&osrd_req);
	if (res == -1)
		goto error;

	rcp_packet osrd_resp;
	res = rcp_recv(&osrd_resp);
	if (res == -1)
		goto error;

	return 0;

error:
	ERROR("send_osrd()");
	return -1;
}

int ptz_available(rcp_session* session)
{
	rcp_packet ptz_req;
	int res;

	init_rcp_header(&ptz_req, session, RCP_COMMAND_CONF_PTZ_CONTROLLER_AVAILABLE, RCP_COMMAND_MODE_READ, RCP_DATA_TYPE_F_FLAG);

	res = rcp_send(&ptz_req);
	if (res == -1)
		goto error;

	rcp_packet ptz_resp;
	res = rcp_recv(&ptz_resp);
	if (res == -1)
		goto error;

	return ptz_resp.payload[0];

error:
	ERROR("send_osrd()");
	return -1;
}

int move_right(rcp_session* session, int speed)
{
	VarSpeedPTZ ptz;
	memset(&ptz, 0, sizeof(VarSpeedPTZ));
	ptz.pan_right = 1;
	ptz.pan_speed = speed;

	return send_osrd(session, 1, 5, (unsigned char*)&ptz, sizeof(VarSpeedPTZ));
}

int move_left(rcp_session* session, int speed)
{
	VarSpeedPTZ ptz;
	memset(&ptz, 0, sizeof(VarSpeedPTZ));
	ptz.pan_left = 1;
	ptz.pan_speed = speed;

	return send_osrd(session, 1, 5, (unsigned char*)&ptz, sizeof(VarSpeedPTZ));
}

int move_up(rcp_session* session, int speed)
{
	VarSpeedPTZ ptz;
	memset(&ptz, 0, sizeof(VarSpeedPTZ));
	ptz.tilt_up = 1;
	ptz.tilt_speed = speed;

	return send_osrd(session, 1, 5, (unsigned char*)&ptz, sizeof(VarSpeedPTZ));
}

int move_down(rcp_session* session, int speed)
{
	VarSpeedPTZ ptz;
	memset(&ptz, 0, sizeof(VarSpeedPTZ));
	ptz.tilt_down = 1;
	ptz.tilt_speed = speed;

	return send_osrd(session, 1, 5, (unsigned char*)&ptz, sizeof(VarSpeedPTZ));
}

int move_stop(rcp_session* session)
{
	VarSpeedPTZ ptz;
	memset(&ptz, 0, sizeof(VarSpeedPTZ));

	return send_osrd(session, 1, 5, (unsigned char*)&ptz, sizeof(VarSpeedPTZ));
}

int zoom_in(rcp_session* session, int speed)
{
	VarSpeedPTZ ptz;
	memset(&ptz, 0, sizeof(VarSpeedPTZ));
	ptz.zoom_in = 1;
	ptz.zoom_speed = speed;

	return send_osrd(session, 1, 5, (unsigned char*)&ptz, sizeof(VarSpeedPTZ));
}

int zoom_out(rcp_session* session, int speed)
{
	VarSpeedPTZ ptz;
	memset(&ptz, 0, sizeof(VarSpeedPTZ));
	ptz.zoom_out = 1;
	ptz.zoom_speed = speed;

	return send_osrd(session, 1, 5, (unsigned char*)&ptz, sizeof(VarSpeedPTZ));
}

int focus_far(rcp_session* session)
{
	VarSpeedPTZ ptz;
	memset(&ptz, 0, sizeof(VarSpeedPTZ));
	ptz.focus_far = 1;

	return send_osrd(session, 1, 5, (unsigned char*)&ptz, sizeof(VarSpeedPTZ));
}

int focus_near(rcp_session* session)
{
	VarSpeedPTZ ptz;
	memset(&ptz, 0, sizeof(VarSpeedPTZ));
	ptz.focus_near = 1;

	return send_osrd(session, 1, 5, (unsigned char*)&ptz, sizeof(VarSpeedPTZ));
}

int iris_darker(rcp_session* session)
{
	VarSpeedPTZ ptz;
	memset(&ptz, 0, sizeof(VarSpeedPTZ));
	ptz.iris_darker = 1;

	return send_osrd(session, 1, 5, (unsigned char*)&ptz, sizeof(VarSpeedPTZ));
}

int iris_brighter(rcp_session* session)
{
	VarSpeedPTZ ptz;
	memset(&ptz, 0, sizeof(VarSpeedPTZ));
	ptz.iris_brighter = 1;

	return send_osrd(session, 1, 5, (unsigned char*)&ptz, sizeof(VarSpeedPTZ));
}
