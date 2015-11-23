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
#include <tlog/tlog.h>

#include "coder.h"
#include "rcpdefs.h"
#include "rcpcommand.h"

int get_coder_preset(int coder)
{
	rcp_packet mp4_req;

	init_rcp_header(&mp4_req, 0, RCP_COMMAND_CONF_MPEG4_CURRENT_PARAMS, RCP_COMMAND_MODE_READ, RCP_DATA_TYPE_T_DWORD);

	// XXX : seems to fix the problem,
	mp4_req.numeric_descriptor = coder + 1;

	rcp_packet* mp4_resp = rcp_command(&mp4_req);
	if (mp4_resp == NULL)
		goto error;

	//log_hex(LOG_DEBUG, "mp4 conf", mp4_resp.payload, mp4_resp.payload_length);
	return ntohl(*(unsigned int*)mp4_resp->payload);

error:
	TL_ERROR("get_mp4_current_config()");
	return -1;
}

int set_coder_preset(int coder, int preset)
{
	rcp_packet preset_req;

	init_rcp_header(&preset_req, 0, RCP_COMMAND_CONF_MPEG4_CURRENT_PARAMS, RCP_COMMAND_MODE_READ, RCP_DATA_TYPE_T_DWORD);
	preset_req.numeric_descriptor = coder;

	unsigned int tmp32 = htonl(preset);
	memcpy(preset_req.payload, &tmp32, 4);
	preset_req.payload_length = 4;

	rcp_packet* preset_resp = rcp_command(&preset_req);
	if (preset_resp == NULL)
		goto error;

	return 0;

error:
	TL_ERROR("set_coder_preset()");
	return -1;
}

int get_coder_video_operation_mode(int coder, int *mode)
{
	rcp_packet mode_req;

	init_rcp_header(&mode_req, 0, RCP_COMMAND_CONF_CODER_VIDEO_OPERATION_MODE, RCP_COMMAND_MODE_READ, RCP_DATA_TYPE_T_DWORD);
	mode_req.numeric_descriptor = coder;

	rcp_packet* mode_resp = rcp_command(&mode_req);
	if (mode_resp == NULL)
		goto error;

	*mode = ntohl(*(unsigned int*)mode_resp->payload);

	return 0;

error:
	TL_ERROR("get_coder_video_operation_mode()");
	return -1;
}

int set_coder_video_operation_mode(int coder, int mode)
{
	rcp_packet mode_req;

	init_rcp_header(&mode_req, 0, RCP_COMMAND_CONF_CODER_VIDEO_OPERATION_MODE, RCP_COMMAND_MODE_WRITE, RCP_DATA_TYPE_T_DWORD);
	mode_req.numeric_descriptor = coder;

	unsigned int tmp32 = htonl(mode);
	memcpy(mode_req.payload, &tmp32, 4);
	mode_req.payload_length = 4;

	rcp_packet* mode_resp = rcp_command(&mode_req);
	if (mode_resp == NULL)
		goto error;

	int res = ntohl(*(unsigned int*)mode_resp->payload);
	if (res != mode)
		goto error;

	return res;

error:
	TL_ERROR("set_coder_video_operation_mode()");
	return -1;
}

int get_h264_encoder_video_operation_mode(int mode[])
{
	rcp_packet mode_req;

	init_rcp_header(&mode_req, 0, RCP_COMMAND_CONF_VIDEO_H264_ENC_BASE_OPERATION_MODE, RCP_COMMAND_MODE_READ, RCP_DATA_TYPE_P_OCTET);
	mode_req.numeric_descriptor = 0;

	rcp_packet* mode_resp = rcp_command(&mode_req);
	if (mode_resp == NULL)
		goto error;

	mode[0] = ntohl(*(unsigned int*)mode_resp->payload);
	mode[1] = ntohl(*(unsigned int*)(mode_resp->payload+4));

	return 0;

error:
	TL_ERROR("get_h264_encoder_video_operation_mode()");
	return -1;
}

int set_h264_encoder_video_operation_mode(int mode[])
{
	rcp_packet mode_req;

	init_rcp_header(&mode_req, 0, RCP_COMMAND_CONF_VIDEO_H264_ENC_BASE_OPERATION_MODE, RCP_COMMAND_MODE_WRITE, RCP_DATA_TYPE_P_OCTET);
	mode_req.numeric_descriptor = 0;

	unsigned int tmp32 = htonl(mode[0]);
	memcpy(mode_req.payload, &tmp32, 4);
	tmp32 = htonl(mode[1]);
	memcpy(mode_req.payload+4, &tmp32, 4);
	mode_req.payload_length = 8;

	rcp_packet* mode_resp = rcp_command(&mode_req);
	if (mode_resp == NULL)
		goto error;

	return 0;

error:
	TL_ERROR("set_h264_encoder_video_operation_mode()");
	return -1;
}

int get_resolution_from_h264_operation_mode(int mode, int *width, int *height, const char** name)
{
	switch (mode)
	{
		case RCP_H264_OPERATION_MODE_MP_SD:
		case RCP_H264_OPERATION_MODE_MP_SD_ROI_PTZ:
		case RCP_H264_OPERATION_MODE_MP_SD_UPRIGHT:
		case RCP_H264_OPERATION_MODE_MP_SD_4CIF_4_3:
		case RCP_H264_OPERATION_MODE_MP_SD_DUAL_IND_ROI:
			*width = 704;
			*height = 576;
			*name = "4CIF";
		break;
		case RCP_H264_OPERATION_MODE_MP_FIXED_720P:
		case RCP_H264_OPERATION_MODE_MP_FIXED_720P_FULL:
		case RCP_H264_OPERATION_MODE_MP_FIXED_720P_SKIP3:
		case RCP_H264_OPERATION_MODE_MP_FIXED_720P_SKIP4:
		case RCP_H264_OPERATION_MODE_MP_FIXED_720P_SKIP7:
		case RCP_H264_OPERATION_MODE_MP_HD_720p:
			*width = 1280;
			*height = 720;
			*name = "HD";
		break;
		case RCP_H264_OPERATION_MODE_MP_FIXED_1080P:
			*width = 1920;
			*height = 1080;
			*name = "FullHD";
		break;
		case RCP_H264_OPERATION_MODE_MP_HD_2592x1944:
			*width = 2592;
			*height = 1944;
			*name = "5MP 4:3";
		break;
		case RCP_H264_OPERATION_MODE_MP_HD_1280x960:
			*width = 1280;
			*height = 960;
			*name = "SXGA-";
		break;
		case RCP_H264_OPERATION_MODE_MP_HD_1440x1080:
			*width = 1440;
			*height = 1080;
			*name = "FullHD Subsampled";
		break;
		case RCP_H264_OPERATION_MODE_MP_HD_1280x1024:
			*width = 1280;
			*height = 1024;
			*name = "SXGA";
		break;
		case RCP_H264_OPERATION_MODE_MP_576x1024:
			*width = 576;
			*height = 1024;
			*name = "576x1024";
		break;
		case RCP_H264_OPERATION_MODE_MP_HD_2704x2032:
			*width = 2704;
			*height = 2032;
			*name = "5MP 4:3";
		break;
		case RCP_H264_OPERATION_MODE_MP_HD_2992x1680:
			*width = 2992;
			*height = 1680;
			*name = "5MP 16:9";
		break;
		case RCP_H264_OPERATION_MODE_MP_HD_3840x2160:
			*width = 3840;
			*height = 2160;
			*name = "UHD 4K";
		break;
		case RCP_H264_OPERATION_MODE_MP_HD_4000x3000:
			*width = 4000;
			*height = 3000;
			*name = "4000x3000";
		break;
		case RCP_H264_OPERATION_MODE_MP_HD_3584x2016:
			*width = 3584;
			*height = 2016;
			*name = "3584x2016";
		break;
		case RCP_H264_OPERATION_MODE_MP_HD_800x600:
			*width = 800;
			*height = 600;
			*name = "SVGA";
		break;
		case RCP_H264_OPERATION_MODE_MP_HD_1024x768:
			*width = 1024;
			*height = 768;
			*name = "XGA";
		break;
		case RCP_H264_OPERATION_MODE_MP_HD_1280x960_C:
			*width = 1280;
			*height = 960;
			*name = "SXGA-";
		break;
		case RCP_H264_OPERATION_MODE_MP_HD_1600x1200:
			*width = 1600;
			*height = 1200;
			*name = "UXGA";
		break;
		case RCP_H264_OPERATION_MODE_MP_HD_3648x2160:
			*width = 3648;
			*height = 2160;
			*name = "3648x2160";
		break;
		case RCP_H264_OPERATION_MODE_MP_HD_2640x2640:
			*width = 2640;
			*height = 2640;
			*name = "2640x2640";
		break;
		case RCP_H264_OPERATION_MODE_MP_HD_1792x1792:
			*width = 1792;
			*height = 1792;
			*name = "1792x1792";
		break;
		case RCP_H264_OPERATION_MODE_MP_HD_1024x1024:
			*width = 1024;
			*height = 1024;
			*name = "1024x1024";
		break;
		case RCP_H264_OPERATION_MODE_MP_HD_800x800_ROI:
			*width = 800;
			*height = 800;
			*name = "800x800";
		break;
		case RCP_H264_OPERATION_MODE_MP_HD_1536x864:
			*width = 1536;
			*height = 864;
			*name = "1536x864";
		break;
		case RCP_H264_OPERATION_MODE_MP_HD_2560x1440:
			*width = 2560;
			*height = 1440;
			*name = "2560x1440";
		break;
		case RCP_H264_OPERATION_MODE_MP_HD_2560x960:
			*width = 2560;
			*height = 960;
			*name = "2560x960";
		break;
		case RCP_H264_OPERATION_MODE_FEDC_MODE_MP_3648x1080:
			*width = 3648;
			*height = 1080;
			*name = "3648x1080";
		break;
		case RCP_H264_OPERATION_MODE_MP_HD_1824x1080:
			*width = 1824;
			*height = 1080;
			*name = "1824x1080";
		break;
		case RCP_H264_OPERATION_MODE_MP_HD_1824x540:
			*width = 1824;
			*height = 540;
			*name = "1824x540";
		break;
		case RCP_H264_OPERATION_MODE_MP_HD_1280x480:
			*width = 1280;
			*height = 480;
			*name = "1280x480";
		break;
		case RCP_H264_OPERATION_MODE_MP_HD_1536x1536:
			*width = 1536;
			*height = 1536;
			*name = "1536x1536";
		break;
		case RCP_H264_OPERATION_MODE_MP_HD_800x800:
			*width = 800;
			*height = 800;
			*name = "800x800";
		break;
		case RCP_H264_OPERATION_MODE_MP_768x768:
			*width = 768;
			*height = 768;
			*name = "768x768";
		break;
		case RCP_H264_OPERATION_MODE_MP_768x288:
			*width = 768;
			*height = 288;
			*name = "768x288";
		break;
		case RCP_H264_OPERATION_MODE_MP_2048x1152:
			*width = 2048;
			*height = 1152;
			*name = "2048x1152";
		break;
		case RCP_H264_OPERATION_MODE_MP_2688x800:
			*width = 2688;
			*height = 800;
			*name = "2688x800";
		break;
		case RCP_H264_OPERATION_MODE_MP_672x200:
			*width = 672;
			*height = 200;
			*name = "672x200";
		break;
		case RCP_H264_OPERATION_MODE_MP_608x360:
			*width = 608;
			*height = 360;
			*name = "608x360";
		break;
		case RCP_H264_OPERATION_MODE_640x240:
			*width = 640;
			*height = 240;
			*name = "640x240";
		break;
		case RCP_H264_OPERATION_MODE_MP_768x576:
			*width = 768;
			*height = 576;
			*name = "768x576";
		break;
		default:
			return -1;
	}
	return 0;
}

#define CODER_SIZE_IN_PAYLOAD	16

int get_coder_list(int coder_type, int media_type, rcp_coder_list* coder_list, int line)
{
	rcp_packet coders_req;

	init_rcp_header(&coders_req, 0, RCP_COMMAND_CONF_RCP_CODER_LIST, RCP_COMMAND_MODE_READ, RCP_DATA_TYPE_P_OCTET);
	coders_req.numeric_descriptor = line;

	coders_req.payload[0] = media_type;
	coders_req.payload[1] = coder_type;
	coders_req.payload[2] = 1;
	coders_req.payload_length = 3;

	rcp_packet* coders_resp = rcp_command(&coders_req);
	if (coders_resp == NULL)
		goto error;

	coder_list->count = coders_resp->payload_length / CODER_SIZE_IN_PAYLOAD;
	coder_list->coder = (rcp_coder*)malloc(sizeof(rcp_coder) * coder_list->count);

	char* pos = coders_resp->payload;
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
	TL_ERROR("coder_list()");
	return -1;
}

static char* coder_cap_str(int cap, int media_type)
{
	switch (media_type)
	{
		case RCP_MEDIA_TYPE_VIDEO:
			switch (cap)
			{
				case RCP_VIDEO_CODING_H263:return "H.263";
				case RCP_VIDEO_CODING_MPEG4:return "Mpeg4";
				case RCP_VIDEO_CODING_MPEG2V:return "Mpeg2 VideoOnly";
				case RCP_VIDEO_CODING_METADATA:return "Meta Data";
				case RCP_VIDEO_CODING_H263P:return "H.263+ 1998";
				case RCP_VIDEO_CODING_H264:return "H.264";
				case RCP_VIDEO_CODING_JPEG:return "Jpeg";
				case RCP_VIDEO_CODING_RECORDED:return "Recorded Media";
				case RCP_VIDEO_CODING_MPEG2S:return "Mpeg2 PrgStr";
			}
		break;

		case RCP_MEDIA_TYPE_AUDIO:
			switch (cap)
			{
				case RCP_AUDIO_CODING_G711:return "G.711";
				case RCP_AUDIO_CODING_MPEG2P:return "Mpeg2 PrgStr";
			}
		break;

		case RCP_MEDIA_TYPE_DATA:
			switch (cap)
			{
				case RCP_DATA_CODING_RCP:return "RCP";
			}
		break;
	}

	return "Unknown";
}

static char* coder_codparam_str(int codparam, int media_type)
{
	switch (media_type)
	{
		case RCP_MEDIA_TYPE_VIDEO:
			switch (codparam)
			{
				case RCP_VIDEO_RESOLUTION_QCIF:return "QCIF";
				case RCP_VIDEO_RESOLUTION_CIF:return "CIF";
				case RCP_VIDEO_RESOLUTION_2CIF:return "2CIF";
				case RCP_VIDEO_RESOLUTION_4CIF:return "4CIF";
				case RCP_VIDEO_RESOLUTION_QVGA:return "QVGA";
				case RCP_VIDEO_RESOLUTION_VGA:return "VGA";
				case RCP_VIDEO_RESOLUTION_HD720:return "HD720";
				case RCP_VIDEO_RESOLUTION_HD1080:return "HD1080";
			}
		break;

		case RCP_MEDIA_TYPE_AUDIO:
			return "none";
		break;

		case RCP_MEDIA_TYPE_DATA:
			switch (codparam)
			{
				case RCP_DATA_CODPARAM_RS232:return "RS232";
				case RCP_DATA_CODPARAM_RS485:return "RS485";
				case RCP_DATA_CODPARAM_RS422:return "RS422";
			}
		break;
	}

	return "Unknown";
}

void log_coder(int level, rcp_coder* coder)
{
	char tmp[200];

	tlog(level, "%-20s %d", "Coder Number", coder->number);
	switch (coder->direction)
	{
		case 0:strcpy(tmp, "Input");break;
		case 1:strcpy(tmp, "Output");break;
		default:strcpy(tmp, "Unknown");break;
	}
	tlog(level, "%-20s %s", "Direction", tmp);
	switch (coder->media_type)
	{
		case RCP_MEDIA_TYPE_AUDIO:strcpy(tmp, "Audio");break;
		case RCP_MEDIA_TYPE_VIDEO:strcpy(tmp, "Video");break;
		case RCP_MEDIA_TYPE_DATA:strcpy(tmp, "Data");break;
		default:strcpy(tmp, "Unknown");break;
	}
	tlog(level, "%-20s %s", "Media Type", tmp);
	tmp[0] = 0;
	for (int i=1; i<=0x8000; i<<=1)
		if ( (coder->caps & i) && strcmp(coder_cap_str(i, coder->media_type), "Unknown")!=0 )
		{
			strcat(tmp, coder_cap_str(i, coder->media_type));
			strcat(tmp, ", ");
		}
	tlog(level, "%-20s %s", "Encoding Caps", tmp);
	tmp[0] = 0;
	for (int i=1; i<=0x8000; i<<=1)
		if ( (coder->current_cap & i) && strcmp(coder_cap_str(i, coder->media_type), "Unknown")!=0 )
		{
			strcat(tmp, coder_cap_str(i, coder->media_type));
			strcat(tmp, ", ");
		}
	tlog(level, "%-20s %s", "Current Cap", tmp);
	tmp[0] = 0;
	for (int i=1; i<=0x1000; i<<=1)
		if ( (coder->param_caps & i) && strcmp(coder_codparam_str(i, coder->media_type), "Unknown")!=0 )
		{
			strcat(tmp, coder_codparam_str(i, coder->media_type));
			strcat(tmp, ", ");
		}
	tlog(level, "%-20s %s", "CodParam Caps", tmp);
	tmp[0] = 0;
	for (int i=1; i<=0x1000; i<<=1)
		if ( (coder->current_param & i) && strcmp(coder_codparam_str(i, coder->media_type), "Unknown")!=0 )
		{
			strcat(tmp, coder_codparam_str(i, coder->media_type));
			strcat(tmp, ", ");
		}
	tlog(level, "%-20s %s", "Current CodParam", tmp);
}
