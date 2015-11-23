/*
 * audio.c
 *
 *  Created on: Oct 22, 2013
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

#include <stddef.h>

#include <tlog/tlog.h>
#include "audio.h"
#include "rcpplus.h"
#include "rcpdefs.h"
#include "rcpcommand.h"

int get_audio_stat()
{
	rcp_packet req;

	init_rcp_header(&req, 0, RCP_COMMAND_CONF_AUDIO_ON_OFF, RCP_COMMAND_MODE_READ, RCP_DATA_TYPE_F_FLAG);
	req.numeric_descriptor = 0;

	rcp_packet* resp = rcp_command(&req);
	if (resp == NULL)
		goto error;

	return resp->payload[0];

error:
	Error("get_audio_stat()");
	return -1;
}

int set_audio_stat(int on_off)
{
	rcp_packet req;

	init_rcp_header(&req, 0, RCP_COMMAND_CONF_AUDIO_ON_OFF, RCP_COMMAND_MODE_WRITE, RCP_DATA_TYPE_F_FLAG);
	req.numeric_descriptor = 0;
	req.payload[0] = on_off;
	req.payload_length = 1;

	rcp_packet* resp = rcp_command(&req);
	if (resp == NULL)
		goto error;

	return 0;

error:
	Error("set_audio_stat()");
	return -1;
}

int get_audio_input_level(int line)
{
	rcp_packet req;

	init_rcp_header(&req, 0, RCP_COMMAND_CONF_AUDIO_INPUT_LEVEL, RCP_COMMAND_MODE_READ, RCP_DATA_TYPE_T_DWORD);
	req.numeric_descriptor = line;

	rcp_packet* resp = rcp_command(&req);
	if (resp == NULL)
		goto error;

	tlog_hex(TLOG_INFO, "audio input level", resp->payload, resp->payload_length);

	return ntohl(*resp->payload);

error:
	TL_ERROR("get_audio_input_level()");
	return -1;
}

int set_audio_input_level(int line, int level)
{
	rcp_packet req;

	init_rcp_header(&req, 0, RCP_COMMAND_CONF_AUDIO_INPUT_LEVEL, RCP_COMMAND_MODE_WRITE, RCP_DATA_TYPE_T_DWORD);
	req.numeric_descriptor = line;
	*(unsigned long*)req.payload = htonl(level);
	req.payload_length = 4;

	rcp_packet* resp = rcp_command(&req);
	if (resp == NULL)
		goto error;

	tlog_hex(TLOG_INFO, "audio input level", resp->payload, resp->payload_length);

	return ntohl(*resp->payload);

error:
	TL_ERROR("set_audio_input_level()");
	return -1;
}

int get_audio_input(int line)
{
	rcp_packet req;

	init_rcp_header(&req, 0, RCP_COMMAND_CONF_AUDIO_INPUT, RCP_COMMAND_MODE_READ, RCP_DATA_TYPE_T_DWORD);
	req.numeric_descriptor = line;

	rcp_packet* resp = rcp_command(&req);
	if (resp == NULL)
		goto error;

	tlog_hex(TLOG_INFO, "audio input", resp->payload, resp->payload_length);

	return ntohl(*resp->payload);

error:
	TL_ERROR("get_audio_input()");
	return -1;
}

int set_audio_input(int line, int input)
{
	rcp_packet req;

	init_rcp_header(&req, 0, RCP_COMMAND_CONF_AUDIO_INPUT, RCP_COMMAND_MODE_WRITE, RCP_DATA_TYPE_T_DWORD);
	req.numeric_descriptor = line;
	*(unsigned long*)req.payload = htonl(input);
	req.payload_length = 4;

	rcp_packet* resp = rcp_command(&req);
	if (resp == NULL)
		goto error;

	tlog_hex(TLOG_INFO, "audio input", resp->payload, resp->payload_length);

	return ntohl(*resp->payload);

error:
	TL_ERROR("set_audio_input()");
	return -1;
}

int get_max_audio_input_level(int line)
{
	rcp_packet req;

	init_rcp_header(&req, 0, RCP_COMMAND_CONF_AUDIO_INPUT_MAX, RCP_COMMAND_MODE_READ, RCP_DATA_TYPE_T_DWORD);
	req.numeric_descriptor = line;

	rcp_packet* resp = rcp_command(&req);
	if (resp == NULL)
		goto error;

	tlog_hex(TLOG_INFO, "audio input max", resp->payload, resp->payload_length);

	return ntohl(*resp->payload);

error:
	TL_ERROR("get_max_audio_input_level()");
	return -1;
}

int get_mic_level(int line)
{
	rcp_packet req;

	init_rcp_header(&req, 0, RCP_COMMAND_CONF_AUDIO_MIC_LEVEL, RCP_COMMAND_MODE_READ, RCP_DATA_TYPE_T_DWORD);
	req.numeric_descriptor = line;

	rcp_packet* resp = rcp_command(&req);
	if (resp == NULL)
		goto error;

	tlog_hex(TLOG_INFO, "audio mic level", resp->payload, resp->payload_length);

	return ntohl(*resp->payload);

error:
	TL_ERROR("get_mic_level()");
	return -1;
}

int set_mic_level(int line, int level)
{
	rcp_packet req;

	init_rcp_header(&req, 0, RCP_COMMAND_CONF_AUDIO_MIC_LEVEL, RCP_COMMAND_MODE_READ, RCP_DATA_TYPE_T_DWORD);
	req.numeric_descriptor = line;
	*(unsigned long*)req.payload = htonl(level);
	req.payload_length = 4;

	rcp_packet* resp = rcp_command(&req);
	if (resp == NULL)
		goto error;

	tlog_hex(TLOG_INFO, "audio mic level", resp->payload, resp->payload_length);

	return ntohl(*resp->payload);

error:
	TL_ERROR("set_mic_level()");
	return -1;
}

int get_max_mic_level(int line)
{
	rcp_packet req;

	init_rcp_header(&req, 0, RCP_COMMAND_CONF_AUDIO_MIC_MAX, RCP_COMMAND_MODE_READ, RCP_DATA_TYPE_T_DWORD);
	req.numeric_descriptor = line;

	rcp_packet* resp = rcp_command(&req);
	if (resp == NULL)
		goto error;

	tlog_hex(TLOG_INFO, "audio mic max", resp->payload, resp->payload_length);

	return ntohl(*resp->payload);

error:
	TL_ERROR("get_max_mic_level()");
	return -1;
	return 0;
}

int get_audio_options(int line)
{
	rcp_packet req;

	init_rcp_header(&req, 0, RCP_COMMAND_CONF_AUDIO_OPTIONS, RCP_COMMAND_MODE_READ, RCP_DATA_TYPE_T_DWORD);
	req.numeric_descriptor = line;

	rcp_packet* resp = rcp_command(&req);
	if (resp == NULL)
		goto error;

	tlog_hex(TLOG_INFO, "audio options", resp->payload, resp->payload_length);

	return ntohl(*resp->payload);

error:
	TL_ERROR("get_audio_options()");
	return -1;
}

int get_audio_input_peek(int line)
{
	rcp_packet req;

	init_rcp_header(&req, 0, RCP_COMMAND_CONF_AUDIO_INPUT_PEEK, RCP_COMMAND_MODE_READ, RCP_DATA_TYPE_T_DWORD);
	req.numeric_descriptor = line;

	rcp_packet* resp = rcp_command(&req);
	if (resp == NULL)
		goto error;

	tlog_hex(TLOG_INFO, "audio input peek", resp->payload, resp->payload_length);

	return ntohl(*resp->payload);

error:
	TL_ERROR("get_max_audio_input_peek()");
	return -1;
}

int set_audio_input_peek(int line, int peek)
{
	rcp_packet req;

	init_rcp_header(&req, 0, RCP_COMMAND_CONF_AUDIO_INPUT_PEEK, RCP_COMMAND_MODE_READ, RCP_DATA_TYPE_T_DWORD);
	req.numeric_descriptor = line;
	*(unsigned long*)req.payload = htonl(peek);
	req.payload_length = 4;

	rcp_packet* resp = rcp_command(&req);
	if (resp == NULL)
		goto error;

	tlog_hex(TLOG_INFO, "audio input peek", resp->payload, resp->payload_length);

	return ntohl(*resp->payload);

error:
	TL_ERROR("set_max_audio_input_peek()");
	return -1;
}
