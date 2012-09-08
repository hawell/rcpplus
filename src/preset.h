/*
 * preset.h
 *
 *  Created on: Sep 5, 2012
 *      Author: arash
 */

#ifndef PRESET_H_
#define PRESET_H_

#include "rcpdefs.h"
#include "rcpplus.h"

#define PRESET_RESOLUTION_QCIF 0
#define PRESET_RESOLUTION_CIF  1
#define PRESET_RESOLUTION_2CIF 2
#define PRESET_RESOLUTION_4CIF 3
#define PRESET_RESOLUTION_12D1 4
#define PRESET_RESOLUTION_23D1 5
#define PRESET_RESOLUTION_QVGA 6
#define PRESET_RESOLUTION_VGA  7
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

int get_current_preset(rcp_session* session, int coder);

int get_preset(rcp_session* session, int preset_id, rcp_mpeg4_preset* preset, int basic);
int set_preset(rcp_session* session, int preset_id, rcp_mpeg4_preset* preset, int basic);

int preset_set_default(rcp_session* session, int preset_id);

int get_preset_name(rcp_session* session, int preset_id, char* name);
int set_preset_name(rcp_session* session, int preset_id, char* name);

int get_preset_param(rcp_session* session, int preset_id, int param, int *value);
int set_preset_param(rcp_session* session, int preset_id, int param, int value);

void log_preset(int level, rcp_mpeg4_preset* preset, int basic);

#endif /* PRESET_H_ */
