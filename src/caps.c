/*
 * caps.c
 *
 *  Created on: Dec 29, 2013
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
#include <stdio.h>
#include <string.h>

#include <tlog/tlog.h>

#include "caps.h"
#include "rcpdefs.h"
#include "rcpcommand.h"


int get_capability_list(cap_list* caps)
{
	rcp_packet req;

	init_rcp_header(&req, 0, RCP_COMMAND_CONF_CAPABILITY_LIST, RCP_COMMAND_MODE_READ, RCP_DATA_TYPE_P_OCTET);

	rcp_packet* resp = rcp_command(&req);
	if (resp == NULL)
		goto error;

	//tlog_hex(TLOG_INFO, "cap list response", resp->payload, resp->payload_length);

	caps->version = ntohs(*(unsigned short*)(resp->payload + 2));
	caps->section_count = ntohs(*(unsigned short*)(resp->payload + 4));
	caps->sections = malloc(sizeof(cap_section) * caps->section_count);
	char* pos = resp->payload + 6;
	for (int i=0; i<caps->section_count; i++)
	{
		caps->sections[i].section_type = ntohs(*(unsigned short*)pos);
		pos += 2;
		caps->sections[i].section_size = ntohs(*(unsigned short*)pos);
		pos += 2;
		caps->sections[i].n_elems = ntohs(*(unsigned short*)pos);
		pos += 2;
		caps->sections[i].elements = malloc(sizeof(cap_element) * caps->sections[i].n_elems);
		for (int j=0; j<caps->sections[i].n_elems; j++)
		{
			caps->sections[i].elements[j].element_type = ntohs(*(unsigned short*)pos);
			pos += 2;
			switch (caps->sections[i].section_type)
			{
				case CAP_SECTION_TYPE_VIDEO:
					caps->sections[i].elements[j].identifier = ntohs(*(unsigned short*)pos);
					pos += 2;
					caps->sections[i].elements[j].compression = ntohs(*(unsigned short*)pos);
					pos += 2;
					caps->sections[i].elements[j].input_number = ntohs(*(unsigned short*)pos);
					pos += 2;
					caps->sections[i].elements[j].resolution = ntohs(*(unsigned short*)pos);
					pos += 2;
				break;
				case CAP_SECTION_TYPE_AUDIO:
					caps->sections[i].elements[j].identifier = ntohs(*(unsigned short*)pos);
					pos += 2;
					caps->sections[i].elements[j].compression = ntohs(*(unsigned short*)pos);
					pos += 2;
				break;
				case CAP_SECTION_TYPE_IO:
				case CAP_SECTION_TYPE_SERIAL:
					caps->sections[i].elements[j].identifier = ntohs(*(unsigned short*)pos);
					pos += 2;
				break;
				case CAP_SECTION_TYPE_CAMDATA:
					caps->sections[i].elements[j].input_number = ntohs(*(unsigned short*)pos);
					pos += 2;
				break;
			}
		}
	}

	return 0;

error:
	TL_ERROR("capability_list()");
	return -1;
}

void free_cap_list(cap_list* caps)
{
	for (int i=0; i<caps->section_count; i++)
		{
			cap_section* sec = &caps->sections[i];
			free(sec->elements);
		}
	free(caps->sections);
}

static const char* cap_section_str(int section)
{
	switch (section)
	{
		case CAP_SECTION_TYPE_VIDEO:
			return "Video";
		case CAP_SECTION_TYPE_AUDIO:
			return "Audio";
		case CAP_SECTION_TYPE_SERIAL:
			return "Serial";
		case CAP_SECTION_TYPE_IO:
			return "IO";
		case CAP_SECTION_TYPE_CAMDATA:
			return "Camera Data";
		default:
			return "Unknown Section";
	}
}

static const char* video_element_type_str(int type)
{
	switch (type)
	{
		case CAP_ELEMENT_VIDEO_ENCODER:
			return "Video Encoder";
		case CAP_ELEMENT_VIDEO_DECODER:
			return "Video Decoder";
		default:
			return "Unknown Element";
	}
}

static const char* video_element_comp_str(int comp)
{
	switch (comp)
	{
		case VIDEO_COMP_MPEG2:
			return "Mpeg2";
		case VIDEO_COMP_MPEG4:
			return "Mpeg4";
		case VIDEO_COMP_H264:
			return "H264";
		case VIDEO_COMP_JPEG:
			return "Jpeg";
		default:
			return "Unknown Compression";
	}
}

static const char* video_element_resolution_str(int res)
{
	switch (res)
	{
		case RCP_VIDEO_RESOLUTION_QCIF:
			return "QCIF";
		case RCP_VIDEO_RESOLUTION_CIF:
			return "CIF";
		case RCP_VIDEO_RESOLUTION_2CIF:
			return "2CIF";
		case RCP_VIDEO_RESOLUTION_4CIF:
			return "4CIF";
		case RCP_VIDEO_RESOLUTION_CUSTOM:
			return "Custom";
		case RCP_VIDEO_RESOLUTION_QVGA:
			return "QVGA";
		case RCP_VIDEO_RESOLUTION_VGA:
			return "VGA";
		case RCP_VIDEO_RESOLUTION_WD144:
			return "WD144";
		case RCP_VIDEO_RESOLUTION_WD288:
			return "WD288";
		case RCP_VIDEO_RESOLUTION_WD432:
			return "WD432";
		case RCP_VIDEO_RESOLUTION_HD720:
			return "HD720";
		case RCP_VIDEO_RESOLUTION_HD1080:
			return "HD1080";
		default:
			return "Unknown Resolution";
	}
}

static const char* audio_element_type_str(int type)
{
	switch (type)
	{
		case CAP_ELEMENT_AUDIO_ENCODER:
			return "Audio Encoder";
		case CAP_ELEMENT_AUDIO_DECODER:
			return "Audio Decoder";
		default:
			return "Unknown Audio Type";
	}
}

static const char* audio_element_comp_str(int comp)
{
	switch (comp) {
	case AUDIO_COMP_MPEG2:
		return "Mpeg2";
	case AUDIO_COMP_G711:
		return "G711";
	case AUDIO_COMP_AAC:
		return "AAC";
	default:
		return NULL;
	}
}

static const char* serial_element_type_str(int type)
{
	switch (type)
	{
		case CAP_ELEMENT_SERIAL_RS232:
			return "RS232";
		case CAP_ELEMENT_SERIAL_RS485:
			return "RS485";
		case CAP_ELEMENT_SERIAL_RS422:
			return "RS422";
		default:
			return "Unknown Serial Type";
	}
}

void log_capabilities(int level, cap_list* caps)
{
	char tmp[200];
	tlog(level, "======== Device Caps ========");

	for (int i=0; i<caps->section_count; i++)
	{
		cap_section* sec = &caps->sections[i];
		if (sec->section_type == CAP_SECTION_TYPE_IO || sec->section_type == CAP_SECTION_TYPE_CAMDATA)
			continue;
		tlog(level, "----- %s -----", cap_section_str(sec->section_type));
		for (int j=0; j<sec->n_elems; j++)
		{
			cap_element* el = &sec->elements[j];
			switch (sec->section_type)
			{
				case CAP_SECTION_TYPE_VIDEO:
					tlog(level, "%-20s %s", "Type", video_element_type_str(el->element_type));
					tlog(level, "%-20s %d", "Id", el->identifier);
					tmp[0] = 0;
					for (int i=1; i<=0x8000; i<<=1)
						if (el->compression & i)
						{
							strcat(tmp, video_element_comp_str(i));
							strcat(tmp, ", ");
						}
					tlog(level, "%-20s %s", "Compression", tmp);
					tlog(level, "%-20s %d", "Input Number", el->input_number);
					tmp[0] = 0;
					for (int i=1; i<=0x8000; i<<=1)
						if (el->resolution & i)
						{
							strcat(tmp, video_element_resolution_str(i));
							strcat(tmp, ", ");
						}
					tlog(level, "%-20s %s", "Resolution", tmp);
				break;
				case CAP_SECTION_TYPE_AUDIO:
					tlog(level, "%-20s %s", "Type", audio_element_type_str(el->element_type));
					tlog(level, "%-20s %d", "Id", el->identifier);
					tmp[0] = 0;
					for (int i = 1; i <= 8000; i <<=1)
						if (el->compression & i) {
							const char* comp_str = audio_element_comp_str(i);
							if (comp_str)
								strcat(tmp, comp_str);
							else
								sprintf(tmp, "0x%04x", i);
							strcat(tmp, ", ");
						}
					tlog(level, "%-20s %s", "Compression", tmp);
				break;
				case CAP_SECTION_TYPE_SERIAL:
					tlog(level, "%-20s %d", "Id", el->identifier);
					tmp[0] = 0;
					for (int i=1; i<=8000; i<<=1)
						if (el->element_type & i)
						{
							strcat(tmp, serial_element_type_str(i));
							strcat(tmp, ", ");
						}
					tlog(level, "%-20s %s", "Type", tmp);
				break;
/*
				case CAP_SECTION_TYPE_IO:
					tlog(level, "Type %-20d", el->element_type);
					tlog(level, "Id %-20d", el->identifier);
				break;
				case CAP_SECTION_TYPE_CAMDATA:
					tlog(level, "Type %-20d", el->element_type);
					tlog(level, "Input Number %-20d", el->input_number);
				break;
*/
			}
			tlog(level, "--------------------------");
		}
		tlog(level, "=============================");
	}

}
