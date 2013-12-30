/*
 * preset.c
 *
 *  Created on: Sep 5, 2012
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

#include "preset.h"
#include "rcpcommand.h"

int get_preset(int preset_id, rcp_mpeg4_preset* preset, int basic)
{
	get_preset_name(preset_id, preset->name);
	get_preset_param(preset_id, RCP_COMMAND_CONF_MPEG4_BANDWIDTH_KBPS, &preset->bandwidth);
	get_preset_param(preset_id, RCP_COMMAND_CONF_MPEG4_BANDWIDTH_KBPS_SOFT_LIMIT, &preset->bandwidth_soft_limit);
	get_preset_param(preset_id, RCP_COMMAND_CONF_MPEG4_BANDWIDTH_KBPS_HARD_LIMIT, &preset->bandwidth_hard_limit);
	get_preset_param(preset_id, RCP_COMMAND_CONF_MPEG4_FRAME_SKIP_RATIO, &preset->frameskip_ratio);
	get_preset_param(preset_id, RCP_COMMAND_CONF_MPEG4_INTRA_FRAME_DISTANCE, &preset->iframe_distance);
	get_preset_param(preset_id, RCP_COMMAND_CONF_VIDEO_QUALITY, &preset->video_quality);
	get_preset_param(preset_id, RCP_COMMAND_CONF_MPEG4_RESOLUTION, &preset->resolution);
	get_preset_param(preset_id, RCP_COMMAND_CONF_MPEG4_FIELD_MODE, &preset->field_mode);

	if (!basic)
	{
		get_preset_param(preset_id, RCP_COMMAND_CONF_MPEG4_I_FRAME_QUANT, &preset->iframe_quantizer);
		get_preset_param(preset_id, RCP_COMMAND_CONF_MPEG4_P_FRAME_QUANT, &preset->pframe_quantizer);
		get_preset_param(preset_id, RCP_COMMAND_CONF_MPEG4_AVC_I_FRAME_QUANT, &preset->avc_iframe_qunatizer);
		get_preset_param(preset_id, RCP_COMMAND_CONF_MPEG4_AVC_P_FRAME_QUANT, &preset->avc_pframe_qunatizer);
		get_preset_param(preset_id, RCP_COMMAND_CONF_MPEG4_AVC_P_FRAME_QUANT_MIN, &preset->avc_pframe_qunatizer_min);
		get_preset_param(preset_id, RCP_COMMAND_CONF_MPEG4_AVC_DELTA_IPQUANT, &preset->avc_delta_ipquantizer);
		get_preset_param(preset_id, RCP_COMMAND_CONF_MPEG4_AVC_DEBLOCKING_ENABLE, &preset->avc_deblocking_enabled);
		get_preset_param(preset_id, RCP_COMMAND_CONF_MPEG4_AVC_DEBLOCKING_ALPHA, &preset->avc_deblocking_alpha);
		get_preset_param(preset_id, RCP_COMMAND_CONF_MPEG4_AVC_DEBLOCKING_BETA, &preset->avc_deblocking_beta);
		get_preset_param(preset_id, RCP_COMMAND_CONF_MPEG4_AVC_CHROMA_QUANT_OFF, &preset->avc_chroma_quantisation_offset);
		get_preset_param(preset_id, RCP_COMMAND_CONF_MPEG4_AVC_CODING_MODE, &preset->avc_coding_mode);
		get_preset_param(preset_id, RCP_COMMAND_CONF_MPEG4_AVC_GOP_STRUCTURE, &preset->avc_gop_structure);
		get_preset_param(preset_id, RCP_COMMAND_CONF_MPEG4_AVC_CABAC, &preset->avc_cabac);
	}

	return 0;
}

int set_preset(int preset_id, rcp_mpeg4_preset* preset, int basic)
{
	set_preset_name(preset_id, preset->name);
	set_preset_param(preset_id, RCP_COMMAND_CONF_MPEG4_BANDWIDTH_KBPS, preset->bandwidth);
	set_preset_param(preset_id, RCP_COMMAND_CONF_MPEG4_BANDWIDTH_KBPS_SOFT_LIMIT, preset->bandwidth_soft_limit);
	set_preset_param(preset_id, RCP_COMMAND_CONF_MPEG4_BANDWIDTH_KBPS_HARD_LIMIT, preset->bandwidth_hard_limit);
	set_preset_param(preset_id, RCP_COMMAND_CONF_MPEG4_FRAME_SKIP_RATIO, preset->frameskip_ratio);
	set_preset_param(preset_id, RCP_COMMAND_CONF_MPEG4_INTRA_FRAME_DISTANCE, preset->iframe_distance);
	set_preset_param(preset_id, RCP_COMMAND_CONF_VIDEO_QUALITY, preset->video_quality);
	set_preset_param(preset_id, RCP_COMMAND_CONF_MPEG4_RESOLUTION, preset->resolution);
	set_preset_param(preset_id, RCP_COMMAND_CONF_MPEG4_FIELD_MODE, preset->field_mode);

	if (!basic)
	{
		set_preset_param(preset_id, RCP_COMMAND_CONF_MPEG4_I_FRAME_QUANT, preset->iframe_quantizer);
		set_preset_param(preset_id, RCP_COMMAND_CONF_MPEG4_P_FRAME_QUANT, preset->pframe_quantizer);
		set_preset_param(preset_id, RCP_COMMAND_CONF_MPEG4_AVC_I_FRAME_QUANT, preset->avc_iframe_qunatizer);
		set_preset_param(preset_id, RCP_COMMAND_CONF_MPEG4_AVC_P_FRAME_QUANT, preset->avc_pframe_qunatizer);
		set_preset_param(preset_id, RCP_COMMAND_CONF_MPEG4_AVC_P_FRAME_QUANT_MIN, preset->avc_pframe_qunatizer_min);
		set_preset_param(preset_id, RCP_COMMAND_CONF_MPEG4_AVC_DELTA_IPQUANT, preset->avc_delta_ipquantizer);
		set_preset_param(preset_id, RCP_COMMAND_CONF_MPEG4_AVC_DEBLOCKING_ENABLE, preset->avc_deblocking_enabled);
		set_preset_param(preset_id, RCP_COMMAND_CONF_MPEG4_AVC_DEBLOCKING_ALPHA, preset->avc_deblocking_alpha);
		set_preset_param(preset_id, RCP_COMMAND_CONF_MPEG4_AVC_DEBLOCKING_BETA, preset->avc_deblocking_beta);
		set_preset_param(preset_id, RCP_COMMAND_CONF_MPEG4_AVC_CHROMA_QUANT_OFF, preset->avc_chroma_quantisation_offset);
		set_preset_param(preset_id, RCP_COMMAND_CONF_MPEG4_AVC_CODING_MODE, preset->avc_coding_mode);
		set_preset_param(preset_id, RCP_COMMAND_CONF_MPEG4_AVC_GOP_STRUCTURE, preset->avc_gop_structure);
		set_preset_param(preset_id, RCP_COMMAND_CONF_MPEG4_AVC_CABAC, preset->avc_cabac);
	}

	return 0;
}

int preset_set_default(int preset_id)
{
	rcp_packet def_req;

	init_rcp_header(&def_req, 0, RCP_COMMAND_CONF_MPEG4_DEFAULTS, RCP_COMMAND_MODE_WRITE, RCP_DATA_TYPE_F_FLAG);
	def_req.numeric_descriptor = preset_id;
	def_req.payload[0] = 0;
	def_req.payload_length = 1;

	rcp_packet* def_resp = rcp_command(&def_req);
	if (def_resp == NULL)
		goto error;

	return 0;

error:
	TL_ERROR("preset_set_default()");
	return -1;
}

int get_preset_name(int preset_id, char* name)
{
	rcp_packet pn_req;

	init_rcp_header(&pn_req, 0, RCP_COMMAND_CONF_MPEG4_NAME, RCP_COMMAND_MODE_READ, RCP_DATA_TYPE_P_STRING);
	pn_req.numeric_descriptor = preset_id;

	rcp_packet* pn_resp = rcp_command(&pn_req);
	if (pn_resp == NULL)
		goto error;

	memcpy(name, pn_resp->payload, pn_resp->payload_length);
	name[pn_resp->payload_length] = 0;

	return 0;

error:
	TL_ERROR("get_preset_name()");
	return -1;
}

int set_preset_name(int preset_id, char* name)
{
	rcp_packet pn_req;

	init_rcp_header(&pn_req, 0, RCP_COMMAND_CONF_MPEG4_NAME, RCP_COMMAND_MODE_WRITE, RCP_DATA_TYPE_P_STRING);
	pn_req.numeric_descriptor = preset_id;

	pn_req.payload_length = strlen(name) + 1;
	memcpy(pn_req.payload, name, pn_req.payload_length);

	rcp_packet* pn_resp = rcp_command(&pn_req);
	if (pn_resp == NULL)
		goto error;

	TL_DEBUG("preset name set to '%s'", pn_resp->payload);
	return 0;

error:
	TL_ERROR("set_preset_name()");
	return -1;
}


int get_param_type(int param)
{
	switch (param)
	{
		case RCP_COMMAND_CONF_MPEG4_BANDWIDTH_KBPS:
		case RCP_COMMAND_CONF_MPEG4_BANDWIDTH_KBPS_SOFT_LIMIT:
		case RCP_COMMAND_CONF_MPEG4_BANDWIDTH_KBPS_HARD_LIMIT:
		case RCP_COMMAND_CONF_MPEG4_INTRA_FRAME_DISTANCE:
		case RCP_COMMAND_CONF_MPEG4_FRAME_SKIP_RATIO:
		case RCP_COMMAND_CONF_MPEG4_RESOLUTION:
		case RCP_COMMAND_CONF_MPEG4_I_FRAME_QUANT:
		case RCP_COMMAND_CONF_MPEG4_P_FRAME_QUANT:
		case RCP_COMMAND_CONF_MPEG4_AVC_I_FRAME_QUANT:
		case RCP_COMMAND_CONF_MPEG4_AVC_P_FRAME_QUANT:
		case RCP_COMMAND_CONF_VIDEO_QUALITY:
			return RCP_DATA_TYPE_T_DWORD;

		case RCP_COMMAND_CONF_MPEG4_FIELD_MODE:
		case RCP_COMMAND_CONF_MPEG4_AVC_CODING_MODE:
		case RCP_COMMAND_CONF_MPEG4_AVC_GOP_STRUCTURE:
		case RCP_COMMAND_CONF_MPEG4_AVC_CABAC:
		case RCP_COMMAND_CONF_MPEG4_AVC_P_FRAME_QUANT_MIN:
		case RCP_COMMAND_CONF_MPEG4_AVC_DELTA_IPQUANT:
			return RCP_DATA_TYPE_T_OCTET;

		case RCP_COMMAND_CONF_MPEG4_AVC_DEBLOCKING_ALPHA:
		case RCP_COMMAND_CONF_MPEG4_AVC_DEBLOCKING_BETA:
		case RCP_COMMAND_CONF_MPEG4_AVC_CHROMA_QUANT_OFF:
			return RCP_DATA_TYPE_T_INT;

		case RCP_COMMAND_CONF_MPEG4_AVC_DEBLOCKING_ENABLE:
			return RCP_DATA_TYPE_F_FLAG;

		default:
			TL_ERROR("invalid parameter %d", param);
			return -1;
	}
}

int get_preset_param(int preset_id, int param, int* value)
{
	rcp_packet p_req;

	int data_type = get_param_type(param);
	if (data_type == -1)
		goto error;

	init_rcp_header(&p_req, 0, param, RCP_COMMAND_MODE_READ, data_type);
	p_req.numeric_descriptor = preset_id;

	rcp_packet* p_resp = rcp_command(&p_req);
	if (p_resp == NULL)
		goto error;

	switch (data_type)
	{
		case RCP_DATA_TYPE_F_FLAG:
		case RCP_DATA_TYPE_T_OCTET:
			*value = p_resp->payload[0];
		break;

		case RCP_DATA_TYPE_T_WORD:
			*value = ntohs(*(unsigned short*)p_resp->payload);
		break;

		case RCP_DATA_TYPE_T_INT:
		case RCP_DATA_TYPE_T_DWORD:
			*value = ntohl(*(unsigned int*)p_resp->payload);
		break;
	}
	return 0;

error:
	TL_ERROR("get_preset_param()");
	return -1;
}

int set_preset_param(int preset_id, int param, int value)
{
	rcp_packet p_req;

	int data_type = get_param_type(param);
	if (data_type == -1)
		goto error;

	init_rcp_header(&p_req, 0, param, RCP_COMMAND_MODE_WRITE, data_type);
	p_req.numeric_descriptor = preset_id;

	unsigned char tmp8;
	unsigned short tmp16;
	unsigned int tmp32;
	switch (data_type)
	{
		case RCP_DATA_TYPE_F_FLAG:
		case RCP_DATA_TYPE_T_OCTET:
			tmp8 = value;
			memcpy(p_req.payload, &tmp8, 1);
			p_req.payload_length = 1;
		break;

		case RCP_DATA_TYPE_T_WORD:
			tmp16 = htons(value);
			memcpy(p_req.payload, &tmp16, 2);
			p_req.payload_length = 2;
		break;

		case RCP_DATA_TYPE_T_INT:
		case RCP_DATA_TYPE_T_DWORD:
			tmp32 = htonl(value);
			memcpy(p_req.payload, &tmp32, 4);
			p_req.payload_length = 4;
		break;
	}

	rcp_packet* p_resp = rcp_command(&p_req);
	if (p_resp == NULL)
		goto error;

	return 0;

error:
	TL_ERROR("set_preset_param()");
	return -1;
}

void log_preset(int level, rcp_mpeg4_preset* preset, int basic)
{
	const char* val;
	tlog(level, "%-25s %s", "name", preset->name);
	switch (preset->resolution)
	{
		case PRESET_RESOLUTION_QCIF:val = "QCIF";break;
		case PRESET_RESOLUTION_CIF:val = "CIF";break;
		case PRESET_RESOLUTION_2CIF:val = "2CIF";break;
		case PRESET_RESOLUTION_4CIF:val = "4CIF";break;
		case PRESET_RESOLUTION_12D1:val = "1/2 D1";break;
		case PRESET_RESOLUTION_23D1:val = "2/3 D1";break;
		case PRESET_RESOLUTION_QVGA:val = "QVGA";break;
		case PRESET_RESOLUTION_VGA:val = "VGA";break;
		case PRESET_RESOLUTION_720P:val = "720P";break;
	}
	tlog(level, "%-25s %s", "resolution", val);
	tlog(level, "%-25s %d kbps", "bandwidth", preset->bandwidth);
	tlog(level, "%-25s %d", "frameskip", preset->frameskip_ratio);
	tlog(level, "%-25s %d kbps", "bw soft", preset->bandwidth_soft_limit);
	tlog(level, "%-25s %d kbps", "bw hard", preset->bandwidth_hard_limit);
	tlog(level, "%-25s %d", "iframe dist", preset->iframe_distance);
	switch (preset->field_mode)
	{
		case PRESET_FIELD_MODE_PROGRESSIVE:val = "progressive";break;
		case PRESET_FIELD_MODE_MERGED:val = "merged";break;
		case PRESET_FIELD_MODE_SEPERATED:val = "seperated";break;
	}
	tlog(level, "%-25s %s", "field mode", val);

	if (!basic)
	{
		if (preset->iframe_quantizer)
			tlog(level, "%-25s %d", "iframe quant", preset->iframe_quantizer);
		else
			tlog(level, "%-25s %s", "iframe quant", "auto");
		if (preset->pframe_quantizer)
			tlog(level, "%-25s %d", "pframe quant", preset->pframe_quantizer);
		else
			tlog(level, "%-25s %s", "pframe quant", "auto");
		tlog(level, "%-25s %d", "video quality", preset->video_quality);
		if (preset->avc_iframe_qunatizer)
			tlog(level, "%-25s %d", "avc iframe quant", preset->avc_iframe_qunatizer);
		else
			tlog(level, "%-25s %s", "avc iframe quant", "auto");
		if (preset->avc_pframe_qunatizer)
			tlog(level, "%-25s %d", "avc pframe quant", preset->avc_pframe_qunatizer);
		else
			tlog(level, "%-25s %s", "avc pframe quant", "auto");
		if (preset->avc_pframe_qunatizer_min)
			tlog(level, "%-25s %d", "avc pframe quant min", preset->avc_pframe_qunatizer_min);
		else
			tlog(level, "%-25s %s", "avc pframe quant min", "auto");
		tlog(level, "%-25s %d", "avc i/p delta", preset->avc_delta_ipquantizer);
		tlog(level, "%-25s %d", "avc deblock enabled", preset->avc_deblocking_enabled);
		tlog(level, "%-25s %d", "avc deblock alpha", preset->avc_deblocking_alpha);
		tlog(level, "%-25s %d", "avc deblock beta", preset->avc_deblocking_beta);
		tlog(level, "%-25s %d", "avc chroma offset", preset->avc_chroma_quantisation_offset);
		switch (preset->avc_coding_mode)
		{
			case PRESET_CODING_MODE_FRAME:val="frame";break;
			case PRESET_CODING_MODE_FIELD:val="field";break;
			case PRESET_CODING_MODE_MACRO:val="macro";break;
			case PRESET_CODING_MODE_PICTURE:val="picture";break;
		}
		tlog(level, "%-25s %s", "avc coding mode", val);
		switch (preset->avc_gop_structure)
		{
			case PRESET_GOP_STRUCT_IP:val="IP";break;
			case PRESET_GOP_STRUCT_IBP:val="IBP";break;
			case PRESET_GOP_STRUCT_IBBP:val="IBBP";break;
			case PRESET_GOP_STRUCT_IBBRBP:val="IBBRBP";break;
		}
		tlog(level, "%-25s %s", "avc gop struct", val);
		if (preset->avc_cabac == 0)
			val = "off";
		else
			val = "on";
		tlog(level, "%-25s %s", "avc cabac", val);
	}
}
