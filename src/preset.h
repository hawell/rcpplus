/*
 * preset.h
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

#ifndef PRESET_H_
#define PRESET_H_

#include "rcpdefs.h"
#include "rcpplus.h"

#define RCP_COMMAND_CONF_MPEG4_DEFAULTS						0x0601

#define RCP_COMMAND_CONF_MPEG4_NAME							0x0602
#define RCP_COMMAND_CONF_MPEG4_BANDWIDTH_KBPS				0x0607
#define RCP_COMMAND_CONF_MPEG4_BANDWIDTH_KBPS_SOFT_LIMIT	0x0612
#define RCP_COMMAND_CONF_MPEG4_BANDWIDTH_KBPS_HARD_LIMIT	0x0613
#define RCP_COMMAND_CONF_MPEG4_INTRA_FRAME_DISTANCE			0x0604
#define RCP_COMMAND_CONF_MPEG4_FRAME_SKIP_RATIO				0x0606
#define RCP_COMMAND_CONF_MPEG4_RESOLUTION					0x0608
#define RCP_COMMAND_CONF_MPEG4_FIELD_MODE					0x060e
#define RCP_COMMAND_CONF_MPEG4_I_FRAME_QUANT				0x060a
#define RCP_COMMAND_CONF_MPEG4_P_FRAME_QUANT				0x060b
#define RCP_COMMAND_CONF_MPEG4_PARAMS_MAX_NUM				0x0614
#define RCP_COMMAND_CONF_MPEG4_AVC_I_FRAME_QUANT			0x0615
#define RCP_COMMAND_CONF_MPEG4_AVC_P_FRAME_QUANT			0x0616
#define RCP_COMMAND_CONF_MPEG4_AVC_P_FRAME_QUANT_MIN		0x0620
#define RCP_COMMAND_CONF_MPEG4_AVC_DELTA_IPQUANT			0x0621
#define RCP_COMMAND_CONF_MPEG4_AVC_DEBLOCKING_ENABLE		0x0617
#define RCP_COMMAND_CONF_MPEG4_AVC_DEBLOCKING_ALPHA			0x0618
#define RCP_COMMAND_CONF_MPEG4_AVC_DEBLOCKING_BETA			0x0619
#define RCP_COMMAND_CONF_MPEG4_AVC_CHROMA_QUANT_OFF			0x061a
#define RCP_COMMAND_CONF_MPEG4_AVC_CODING_MODE				0x0a45
#define RCP_COMMAND_CONF_MPEG4_AVC_GOP_STRUCTURE			0x0a94
#define RCP_COMMAND_CONF_MPEG4_AVC_CABAC					0x0aa6
#define RCP_COMMAND_CONF_VIDEO_QUALITY						0x0a82

#define PRESET_RESOLUTION_QCIF 0
#define PRESET_RESOLUTION_CIF  1
#define PRESET_RESOLUTION_2CIF 2
#define PRESET_RESOLUTION_4CIF 3
#define PRESET_RESOLUTION_12D1 4
#define PRESET_RESOLUTION_23D1 5
#define PRESET_RESOLUTION_QVGA 6
#define PRESET_RESOLUTION_VGA  7
#define PRESET_RESOLUTION_WD144 8
#define PRESET_RESOLUTION_WD288 9
#define PRESET_RESOLUTION_WD432 10
#define PRESET_RESOLUTION_720P 12

#define PRESET_FIELD_MODE_PROGRESSIVE 0
#define PRESET_FIELD_MODE_MERGED      1
#define PRESET_FIELD_MODE_SEPERATED   2

#define PRESET_CODING_MODE_FRAME    0
#define PRESET_CODING_MODE_FIELD    1
#define PRESET_CODING_MODE_MACRO    2
#define PRESET_CODING_MODE_PICTURE  3

#define PRESET_GOP_STRUCT_IP       0
#define PRESET_GOP_STRUCT_IBP      1
#define PRESET_GOP_STRUCT_IBBP     2
#define PRESET_GOP_STRUCT_IBBRBP   3

#define PRESET_CABAC_OFF      0
#define PRESET_CABAC_ON       1


typedef struct {
	char name[200];
	int resolution;	                       // 0=QCIF, 1=CIF, 2=2CIF, 3=4CIF, 4=(1/2 D1), 5=(2/3D1), 6=QVGA, 7=VGA, 12=720P
	int bandwidth;                         // kbps
	int frameskip_ratio;                   // 1 = all frames,...

	int bandwidth_soft_limit;              // kbps
	int bandwidth_hard_limit;              // kbps
	int iframe_distance;                   // intra frame distance
	int field_mode;                        // 0=progressive, 1=merged, 2=separated
	int iframe_quantizer;                  // 1-31,0=auto
	int pframe_quantizer;                  // 1-31,0=auto

	int video_quality;

	int avc_iframe_qunatizer;              // 9-51,0=auto
	int avc_pframe_qunatizer;              // 9-51,0=auto
	int avc_pframe_qunatizer_min;          // 9-51,0=auto
	int avc_delta_ipquantizer;             // difference (-10..+10) between I and P-Frame quantization
	int avc_deblocking_enabled;
	int avc_deblocking_alpha;              // alpha H264 deblocking coefficient (-5...5)
	int avc_deblocking_beta;               // beta H264 deblocking coefficent (-5...5)
	int avc_chroma_quantisation_offset;    // chroma quantisation offset (-12...12)
	int avc_coding_mode;                   // 0=frame; 1=field; 2=macro block adaptive ff; 3=picture adaptive ff
	int avc_gop_structure;                 // 0=IP; 1=IBP; 2=IBBP; 3=IBBRBP
	int avc_cabac;                         // 0=off; 1=on
} rcp_mpeg4_preset;

int get_preset(int preset_id, rcp_mpeg4_preset* preset, int basic);
int set_preset(int preset_id, rcp_mpeg4_preset* preset, int basic);

int preset_set_default(int preset_id);

int get_preset_name(int preset_id, char* name);
int set_preset_name(int preset_id, char* name);

int get_preset_param(int preset_id, int param, int *value);
int set_preset_param(int preset_id, int param, int value);

void log_preset(int level, rcp_mpeg4_preset* preset, int basic);

#endif /* PRESET_H_ */
