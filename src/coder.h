/*
 * coder.h
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

#ifndef CODER_H_
#define CODER_H_

#include "rcpplus.h"

#define RCP_VIDEO_CODING_H263		0x0002
#define RCP_VIDEO_CODING_MPEG4		0x0004 // elementary stream
#define RCP_VIDEO_CODING_MPEG2V		0x0008 // only video
#define RCP_VIDEO_CODING_METADATA	0x0010 // output from motion detection
#define RCP_VIDEO_CODING_H263P		0x0020 // H.263+ 1998
#define RCP_VIDEO_CODING_H264		0x0040
#define RCP_VIDEO_CODING_JPEG		0x0080
#define RCP_VIDEO_CODING_RECORDED	0x4000
#define RCP_VIDEO_CODING_MPEG2S		0x8000 // program stream

#define RCP_VIDEO_RESOLUTION_QCIF		0x0001
#define RCP_VIDEO_RESOLUTION_CIF		0x0002
#define RCP_VIDEO_RESOLUTION_2CIF		0x0004
#define RCP_VIDEO_RESOLUTION_4CIF		0x0008
#define RCP_VIDEO_RESOLUTION_QVGA		0x0020
#define RCP_VIDEO_RESOLUTION_VGA		0x0040
#define RCP_VIDEO_RESOLUTION_HD720		0x0080
#define RCP_VIDEO_RESOLUTION_HD1080	0x0100

#define RCP_AUDIO_CODING_G711		0x0001
#define RCP_AUDIO_CODING_MPEG2P		0x8000

#define RCP_DATA_CODING_RCP			0x0001

#define RCP_DATA_CODPARAM_RS232		0x0001
#define RCP_DATA_CODPARAM_RS485		0x0002
#define RCP_DATA_CODPARAM_RS422		0x0004

#define RCP_COMMAND_CONF_RCP_CODER_LIST					0xff11
#define RCP_COMMAND_CONF_CODER_VIDEO_OPERATION_MODE			0x0a9c

#define RCP_CODER_MODE_H263	1
#define RCP_CODER_MODE_H264	2

typedef struct rcp_coder_tag {
	unsigned char media_type;		// video, audio or data
	unsigned char direction;		// input/output
	unsigned short number;			// the absolute coder number
	unsigned short caps;			// all coding capabilities
	unsigned short current_cap;		// current available coding capability
	unsigned short param_caps;		// all CodParameter capabilities (resolution capabilities for video)
	unsigned short current_param;	// current CodParameter capability (current resolution for video)
} rcp_coder;

typedef struct {
	int count;
	rcp_coder *coder;
} rcp_coder_list;

int get_coder_list(int coder_type, int media_type, rcp_coder_list* coder_list);

int get_coder_video_operation_mode(int coder, int *mode);
int set_coder_video_operation_mode(int coder, int mode);

int get_coder_preset(int coder);
int set_coder_preset(int coder, int preset);

void log_coder(int level, rcp_coder* coder);

#endif /* CODER_H_ */
