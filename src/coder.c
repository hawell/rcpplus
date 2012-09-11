/*
 * coder.c
 *
 *  Created on: Sep 10, 2012
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

#include "coder.h"
#include "rcpdefs.h"
#include "rcplog.h"

int get_coder_preset(rcp_session* session, int coder)
{
	rcp_packet mp4_req;
	int res;

	init_rcp_header(&mp4_req, session, RCP_COMMAND_CONF_MPEG4_CURRENT_PARAMS, RCP_COMMAND_MODE_READ, RCP_DATA_TYPE_T_DWORD);
	mp4_req.numeric_descriptor = coder;

	res = rcp_send(&mp4_req);
	if (res == -1)
		goto error;

	rcp_packet mp4_resp;
	res = rcp_recv(&mp4_resp);
	if (res == -1)
		goto error;

	//log_hex(LOG_DEBUG, "mp4 conf", mp4_resp.payload, mp4_resp.payload_length);
	return ntohl(*(unsigned int*)mp4_resp.payload);

error:
	ERROR("get_mp4_current_config()");
	return -1;
}

int set_coder_preset(rcp_session* session, int coder, int preset)
{
	rcp_packet preset_req;
	int res;

	init_rcp_header(&preset_req, session, RCP_COMMAND_CONF_MPEG4_CURRENT_PARAMS, RCP_COMMAND_MODE_READ, RCP_DATA_TYPE_T_DWORD);
	preset_req.numeric_descriptor = coder;

	unsigned int tmp32 = htonl(preset);
	memcpy(preset_req.payload, &tmp32, 4);
	preset_req.payload_length = 4;

	res = rcp_send(&preset_req);
	if (res == -1)
		goto error;

	rcp_packet preset_resp;
	res = rcp_recv(&preset_resp);
	if (res == -1)
		goto error;

	return 0;

error:
	ERROR("set_coder_preset()");
	return -1;
}

int get_coder_video_operation_mode(rcp_session* session, int coder, int *mode)
{
	rcp_packet mode_req;
	int res;

	init_rcp_header(&mode_req, session, RCP_COMMAND_CONF_CODER_VIDEO_OPERATION_MODE, RCP_COMMAND_MODE_READ, RCP_DATA_TYPE_T_DWORD);
	mode_req.numeric_descriptor = coder;

	res = rcp_send(&mode_req);
	if (res == -1)
		goto error;

	rcp_packet mode_resp;
	res = rcp_recv(&mode_resp);
	if (res == -1)
		goto error;

	*mode = ntohl(*(unsigned int*)mode_resp.payload);

	return 0;

error:
	ERROR("get_coder_video_operation_mode()");
	return -1;
}

int set_coder_video_operation_mode(rcp_session* session, int coder, int mode)
{
	rcp_packet mode_req;
	int res;

	init_rcp_header(&mode_req, session, RCP_COMMAND_CONF_CODER_VIDEO_OPERATION_MODE, RCP_COMMAND_MODE_WRITE, RCP_DATA_TYPE_T_DWORD);
	mode_req.numeric_descriptor = coder;

	unsigned int tmp32 = htonl(mode);
	memcpy(mode_req.payload, &tmp32, 4);
	mode_req.payload_length = 4;

	res = rcp_send(&mode_req);
	if (res == -1)
		goto error;

	rcp_packet mode_resp;
	res = rcp_recv(&mode_resp);
	if (res == -1)
		goto error;

	return ntohl(*(unsigned int*)mode_resp.payload);

error:
	ERROR("set_coder_video_operation_mode()");
	return -1;
}

#define CODER_SIZE_IN_PAYLOAD	16

int get_coder_list(rcp_session* session, int coder_type, int media_type, rcp_coder_list* coder_list)
{
	rcp_packet coders_req;
	int res;

	init_rcp_header(&coders_req, session, RCP_COMMAND_CONF_RCP_CODER_LIST, RCP_COMMAND_MODE_READ, RCP_DATA_TYPE_P_OCTET);
	coders_req.numeric_descriptor = 1; // line number - where do we get this?!!

	coders_req.payload[0] = RCP_MEDIA_TYPE_VIDEO;
	coders_req.payload[1] = coder_type;
	coders_req.payload[2] = 1;
	coders_req.payload_length = 3;

	res = rcp_send(&coders_req);
	if (res == -1)
		goto error;

	rcp_packet coders_resp;
	res = rcp_recv(&coders_resp);
	if (res == -1)
		goto error;

	coder_list->count = coders_resp.payload_length / CODER_SIZE_IN_PAYLOAD;
	coder_list->coder = (rcp_coder*)malloc(sizeof(rcp_coder) * coder_list->count);

	unsigned char* pos = coders_resp.payload;
	for (int i=0; i<coder_list->count; i++)
	{
		rcp_coder* c = coder_list->coder + i;
		c->direction = coder_type;
		c->media_type = media_type;
		c->number = ntohs(*(unsigned short*)(pos));
		c->caps = ntohs(*(unsigned short*)(pos + 2));
		c->current_cap = ntohs(*(unsigned short*)(pos + 4));
		c->param_caps = ntohs(*(unsigned short*)(pos + 6));
		c->current_param = ntohs(*(unsigned short*)(pos + 8));
		pos += CODER_SIZE_IN_PAYLOAD;
	}

	return 0;

error:
	ERROR("coder_list()");
	return -1;
}

