/*
 * alarm.c
 *
 *  Created on: Nov 24, 2015
 *      Author: arash
 */

#include <stddef.h>
#include <string.h>

#include <tlog/tlog.h>
#include "audio.h"
#include "rcpplus.h"
#include "rcpdefs.h"
#include "rcpcommand.h"
#include "alarm.h"

unsigned int get_viproc_id(int line)
{
	rcp_packet req;

	init_rcp_header(&req, 0, RCP_COMMAND_CONF_VIPROC_ID, RCP_COMMAND_MODE_READ, RCP_DATA_TYPE_T_DWORD);
	req.numeric_descriptor = line;

	rcp_packet* resp = rcp_command(&req);
	if (resp == NULL)
		goto error;

	return ntohl(*(uint32_t*)resp->payload);

error:
	TL_ERROR("get_viproc_id()");
	return 0;
}

int get_vca_status(int line)
{
	rcp_packet req;

	init_rcp_header(&req, 0, RCP_COMMAND_CONF_ENABLE_VCA, RCP_COMMAND_MODE_READ, RCP_DATA_TYPE_F_FLAG);
	req.numeric_descriptor = line;

	rcp_packet* resp = rcp_command(&req);
	if (resp == NULL)
		goto error;

	tlog_hex(TLOG_INFO, "payload : ", resp->payload, resp->payload_length);

	return resp->payload[0];

error:
	TL_ERROR("set_vca_status()");
	return 0;
}

int set_vca_status(int line, int status)
{
	rcp_packet req;

	init_rcp_header(&req, 0, RCP_COMMAND_CONF_ENABLE_VCA, RCP_COMMAND_MODE_WRITE, RCP_DATA_TYPE_F_FLAG);
	req.numeric_descriptor = line;
	req.payload[0] = status;
	req.payload_length = 1;

	rcp_packet* resp = rcp_command(&req);
	if (resp == NULL)
		goto error;

	tlog_hex(TLOG_INFO, "payload : ", resp->payload, resp->payload_length);

	return 0;

error:
	TL_ERROR("set_vca_status()");
	return -1;
}

int get_viproc_mode(int line)
{
	rcp_packet req;

	init_rcp_header(&req, 0, RCP_COMMAND_CONF_VIPROC_MODE, RCP_COMMAND_MODE_READ, RCP_DATA_TYPE_T_OCTET);
	req.numeric_descriptor = line;

	rcp_packet* resp = rcp_command(&req);
	if (resp == NULL)
		goto error;

	return resp->payload[0];

error:
	TL_ERROR("get_viproc_mode()");
	return -1;
}

int set_viproc_mode(int line, int mode)
{
	rcp_packet req;

	init_rcp_header(&req, 0, RCP_COMMAND_CONF_VIPROC_MODE, RCP_COMMAND_MODE_WRITE, RCP_DATA_TYPE_T_OCTET);
	req.numeric_descriptor = line;
	req.payload[0] = mode;
	req.payload_length = 1;

	rcp_packet* resp = rcp_command(&req);
	if (resp == NULL)
		goto error;

	return 0;

error:
	TL_ERROR("set_viproc_mode()");
	return -1;
}

unsigned int get_viproc_alarm_mask(int line)
{
	rcp_packet req;

	init_rcp_header(&req, 0, RCP_COMMAND_CONF_VIPROC_ALARM_MASK, RCP_COMMAND_MODE_READ, RCP_DATA_TYPE_P_OCTET);
	req.numeric_descriptor = line;

	rcp_packet* resp = rcp_command(&req);
	if (resp == NULL)
		goto error;

	tlog_hex(TLOG_INFO, "mask : ", resp->payload, resp->payload_length);

	return ntohl(*((unsigned int*)resp->payload));

error:
	TL_ERROR("get_viproc_alarm_mask()");
	return -1;
}

int set_viproc_alarm_mask(int line, unsigned int* mask)
{
	rcp_packet req;

	init_rcp_header(&req, 0, RCP_COMMAND_CONF_VIPROC_ALARM_MASK, RCP_COMMAND_MODE_WRITE, RCP_DATA_TYPE_P_OCTET);
	req.numeric_descriptor = line;
	unsigned int mask_n = htonl(*mask);
	memcpy(req.payload, &mask_n, sizeof(unsigned int));
	req.payload_length = 4;

	rcp_packet* resp = rcp_command(&req);
	if (resp == NULL)
		goto error;

	return 0;

error:
	TL_ERROR("set_viproc_alarm_mask()");
	return -1;
}

int viproc_start_edit(int line, unsigned int id)
{
	rcp_packet req;

	init_rcp_header(&req, 0, RCP_COMMAND_CONF_START_VIPROC_CONFIG_EDITING, RCP_COMMAND_MODE_WRITE, RCP_DATA_TYPE_T_DWORD);
	req.numeric_descriptor = line;
	unsigned int id_n = htonl(id);
	memcpy(req.payload, &id_n, sizeof(unsigned int));
	req.payload_length = 4;

	rcp_packet* resp = rcp_command(&req);
	if (resp == NULL)
		goto error;

	return 0;

error:
	TL_ERROR("viproc_start_edit()");
	return -1;
}

int viproc_stop_edit(int line, unsigned int id)
{
	rcp_packet req;

	init_rcp_header(&req, 0, RCP_COMMAND_CONF_STOP_VIPROC_CONFIG_EDITING, RCP_COMMAND_MODE_WRITE, RCP_DATA_TYPE_T_DWORD);
	req.numeric_descriptor = line;
	unsigned int id_n = htonl(id);
	memcpy(req.payload, &id_n, sizeof(unsigned int));
	req.payload_length = 4;

	rcp_packet* resp = rcp_command(&req);
	if (resp == NULL)
		goto error;

	return 0;

error:
	TL_ERROR("viproc_stop_edit()");
	return -1;
}

int get_viproc_sensitive_area(int line, image_grid* grid)
{
	rcp_packet req;

	init_rcp_header(&req, 0, RCP_COMMAND_CONF_VIPROC_SENSITIVE_AREA, RCP_COMMAND_MODE_WRITE, RCP_DATA_TYPE_T_OCTET);
	req.numeric_descriptor = line;

	rcp_packet* resp = rcp_command(&req);
	if (resp == NULL)
		goto error;

	grid->width = ntohs(*(uint16_t*)resp->payload);
	grid->height = ntohs(*(uint16_t*)resp->payload+2);
	grid->cells_x = resp->payload[4];
	grid->cells_y = resp->payload[5];
	grid->steps_x = resp->payload[6];
	grid->steps_y = resp->payload[7];
	grid->start_x = resp->payload[8];
	grid->start_x = resp->payload[9];

	memcpy(grid->bitmask, resp->payload+10, resp->payload_length-10);

	tlog_hex(TLOG_INFO, "payload", resp->payload, resp->payload_length);

	return 0;

error:
	TL_ERROR("get_viproc_sensitive_area()");
	return -1;
}

int set_viproc_sensitive_area(int line, image_grid* grid)
{
	rcp_packet req;

	init_rcp_header(&req, 0, RCP_COMMAND_CONF_VIPROC_SENSITIVE_AREA, RCP_COMMAND_MODE_WRITE, RCP_DATA_TYPE_T_OCTET);
	req.numeric_descriptor = line;

	uint16_t tmp;
	tmp = htons(grid->width);
	memcpy(req.payload, &tmp, 2);
	tmp = htons(grid->height);
	memcpy(req.payload+2, &tmp, 2);
	req.payload[4] = grid->cells_x;
	req.payload[5] = grid->cells_y;
	req.payload[6] = grid->steps_x;
	req.payload[7] = grid->steps_y;
	req.payload[8] = grid->start_x;
	req.payload[9] = grid->start_y;
	int bitmask_bytes = grid->cells_x*grid->cells_y / 8 + 1;
	memcpy(req.payload+10, grid->bitmask, bitmask_bytes);
	req.payload_length = bitmask_bytes + 10;

	rcp_packet* resp = rcp_command(&req);
	if (resp == NULL)
		goto error;

	return 0;

error:
	TL_ERROR("set_viproc_sensitive_area()");
	return -1;
}
