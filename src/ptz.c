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
#include <tlog/tlog.h>

#include "ptz.h"
#include "rcpdefs.h"
#include "rcpcommand.h"

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

static int send_osrd(int lease_time, int opcode, unsigned char* data, int data_len)
{
	rcp_packet osrd_req;

	init_rcp_header(&osrd_req, 0, RCP_COMMAND_CONF_RCP_TRANSFER_TRANSPARENT_DATA, RCP_COMMAND_MODE_WRITE, RCP_DATA_TYPE_P_OCTET);
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

	rcp_packet* osrd_resp = rcp_command(&osrd_req);

	if (osrd_resp == NULL)
		goto error;
	if (osrd_resp->payload[0] == 0)
	{
		TL_ERROR("access to serial port denied");
		goto error;
	}

	return 0;

error:
	TL_ERROR("send_osrd()");
	return -1;
}

int ptz_available()
{
	rcp_packet ptz_req;

	init_rcp_header(&ptz_req, 0, RCP_COMMAND_CONF_PTZ_CONTROLLER_AVAILABLE, RCP_COMMAND_MODE_READ, RCP_DATA_TYPE_F_FLAG);

	rcp_packet* ptz_resp = rcp_command(&ptz_req);
	if (ptz_resp == NULL)
		goto error;

	return ptz_resp->payload[0];

error:
	TL_ERROR("ptz_available()");
	return 0;
}

int move_right(int speed)
{
	VarSpeedPTZ ptz;
	memset(&ptz, 0, sizeof(VarSpeedPTZ));
	ptz.pan_right = 1;
	ptz.pan_speed = speed;

	return send_osrd(1, 5, (unsigned char*)&ptz, sizeof(VarSpeedPTZ));
}

int move_left(int speed)
{
	VarSpeedPTZ ptz;
	memset(&ptz, 0, sizeof(VarSpeedPTZ));
	ptz.pan_left = 1;
	ptz.pan_speed = speed;

	return send_osrd(1, 5, (unsigned char*)&ptz, sizeof(VarSpeedPTZ));
}

int move_up(int speed)
{
	VarSpeedPTZ ptz;
	memset(&ptz, 0, sizeof(VarSpeedPTZ));
	ptz.tilt_up = 1;
	ptz.tilt_speed = speed;

	return send_osrd(1, 5, (unsigned char*)&ptz, sizeof(VarSpeedPTZ));
}

int move_down(int speed)
{
	VarSpeedPTZ ptz;
	memset(&ptz, 0, sizeof(VarSpeedPTZ));
	ptz.tilt_down = 1;
	ptz.tilt_speed = speed;

	return send_osrd(1, 5, (unsigned char*)&ptz, sizeof(VarSpeedPTZ));
}

int move_up_right(int pan_speed, int tilt_speed)
{
	VarSpeedPTZ ptz;
	memset(&ptz, 0, sizeof(VarSpeedPTZ));
	ptz.tilt_up = 1;
	ptz.tilt_speed = tilt_speed;
	ptz.pan_right = 1;
	ptz.pan_speed = pan_speed;

	return send_osrd(1, 5, (unsigned char*)&ptz, sizeof(VarSpeedPTZ));
}

int move_up_left(int pan_speed, int tilt_speed)
{
	VarSpeedPTZ ptz;
	memset(&ptz, 0, sizeof(VarSpeedPTZ));
	ptz.tilt_up = 1;
	ptz.tilt_speed = tilt_speed;
	ptz.pan_left = 1;
	ptz.pan_speed = pan_speed;

	return send_osrd(1, 5, (unsigned char*)&ptz, sizeof(VarSpeedPTZ));
}

int move_down_right(int pan_speed, int tilt_speed)
{
	VarSpeedPTZ ptz;
	memset(&ptz, 0, sizeof(VarSpeedPTZ));
	ptz.tilt_down = 1;
	ptz.tilt_speed = tilt_speed;
	ptz.pan_right = 1;
	ptz.pan_speed = pan_speed;

	return send_osrd(1, 5, (unsigned char*)&ptz, sizeof(VarSpeedPTZ));
}

int move_down_left(int pan_speed, int tilt_speed)
{
	VarSpeedPTZ ptz;
	memset(&ptz, 0, sizeof(VarSpeedPTZ));
	ptz.tilt_down = 1;
	ptz.tilt_speed = tilt_speed;
	ptz.pan_left = 1;
	ptz.pan_speed = pan_speed;

	return send_osrd(1, 5, (unsigned char*)&ptz, sizeof(VarSpeedPTZ));
}

int move_stop()
{
	VarSpeedPTZ ptz;
	memset(&ptz, 0, sizeof(VarSpeedPTZ));

	return send_osrd(1, 5, (unsigned char*)&ptz, sizeof(VarSpeedPTZ));
}

int zoom_in(int speed)
{
	VarSpeedPTZ ptz;
	memset(&ptz, 0, sizeof(VarSpeedPTZ));
	ptz.zoom_in = 1;
	ptz.zoom_speed = speed;

	return send_osrd(1, 5, (unsigned char*)&ptz, sizeof(VarSpeedPTZ));
}

int zoom_out(int speed)
{
	VarSpeedPTZ ptz;
	memset(&ptz, 0, sizeof(VarSpeedPTZ));
	ptz.zoom_out = 1;
	ptz.zoom_speed = speed;

	return send_osrd(1, 5, (unsigned char*)&ptz, sizeof(VarSpeedPTZ));
}

int focus_far()
{
	VarSpeedPTZ ptz;
	memset(&ptz, 0, sizeof(VarSpeedPTZ));
	ptz.focus_far = 1;

	return send_osrd(1, 5, (unsigned char*)&ptz, sizeof(VarSpeedPTZ));
}

int focus_near()
{
	VarSpeedPTZ ptz;
	memset(&ptz, 0, sizeof(VarSpeedPTZ));
	ptz.focus_near = 1;

	return send_osrd(1, 5, (unsigned char*)&ptz, sizeof(VarSpeedPTZ));
}

int iris_darker()
{
	VarSpeedPTZ ptz;
	memset(&ptz, 0, sizeof(VarSpeedPTZ));
	ptz.iris_darker = 1;

	return send_osrd(1, 5, (unsigned char*)&ptz, sizeof(VarSpeedPTZ));
}

int iris_brighter()
{
	VarSpeedPTZ ptz;
	memset(&ptz, 0, sizeof(VarSpeedPTZ));
	ptz.iris_brighter = 1;

	return send_osrd(1, 5, (unsigned char*)&ptz, sizeof(VarSpeedPTZ));
}

int preposition_set(unsigned short preposition_number)
{
	Preposition prep;
	memset(&prep, 0, sizeof(Preposition));
	prep.function_code = 4;
	prep.data_bit_lo = preposition_number & 0x03FF;
	prep.data_bit_hi = (preposition_number >> 7) & 0x07;

	return send_osrd(1, 7, (unsigned char*)&prep, sizeof(prep));
}

int preposition_shot(unsigned short preposition_number)
{
	Preposition prep;
	memset(&prep, 0, sizeof(Preposition));
	prep.function_code = 5;
	prep.data_bit_lo = preposition_number & 0x03FF;
	prep.data_bit_hi = (preposition_number >> 7) & 0x07;

	return send_osrd(1, 7, (unsigned char*)&prep, sizeof(prep));
}
