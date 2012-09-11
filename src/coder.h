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

#define RCP_COMMAND_CONF_RCP_CODER_LIST					0xff11
#define RCP_COMMAND_CONF_CODER_VIDEO_OPERATION_MODE			0x0a9c

typedef struct {
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

int get_coder_list(rcp_session* session, int coder_type, int media_type, rcp_coder_list* coder_list);

int get_coder_video_operation_mode(rcp_session* session, int coder, int *mode);
int set_coder_video_operation_mode(rcp_session* session, int coder, int mode);

int get_coder_preset(rcp_session* session, int coder);
int set_coder_preset(rcp_session* session, int coder, int preset);

#endif /* CODER_H_ */
