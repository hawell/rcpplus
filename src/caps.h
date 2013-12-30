/*
 * caps.h
 *
 *  Created on: Dec 29, 2013
 *      Author: arash
 */

#ifndef CAPS_H_
#define CAPS_H_

#define CAP_SECTION_TYPE_VIDEO		0x0001
#define CAP_SECTION_TYPE_AUDIO		0x0002
#define CAP_SECTION_TYPE_SERIAL	0x0003
#define CAP_SECTION_TYPE_IO		0x0004
#define CAP_SECTION_TYPE_CAMDATA	0x0005

#define CAP_ELEMENT_VIDEO_ENCODER	0x0001
#define CAP_ELEMENT_VIDEO_DECODER	0x0002

#define CAP_ELEMENT_AUDIO_ENCODER	0x0001
#define CAP_ELEMENT_AUDIO_DECODER	0x0002

#define CAP_ELEMENT_SERIAL_RS232	0x0001
#define CAP_ELEMENT_SERIAL_RS485	0x0002
#define CAP_ELEMENT_SERIAL_RS422	0x0004

#define CAP_ELEMENT_IO_INPUT		0x0001
#define CAP_ELEMENT_IO_OUTPUT		0x0002
#define CAP_ELEMENT_IO_VIRTUAL_IN	0x0003

#define CAP_ELEMENT_CAMDATA_BILINX	0x0001

#define AUDIO_COMP_MPEG2	0x0001
#define AUDIO_COMP_G711	0x0002

#define VIDEO_COMP_MPEG2	0x0001
#define VIDEO_COMP_MPEG4	0x0002
#define VIDEO_COMP_H264	0x0004
#define VIDEO_COMP_JPEG	0x0008

#define VIDEO_RESO_QCIF	0x0001
#define VIDEO_RESO_CIF		0x0002
#define VIDEO_RESO_2CIF	0x0004
#define VIDEO_RESO_4CIF	0x0008
#define VIDEO_RESO_CUSTOM	0x0010
#define VIDEO_RESO_QVGA	0x0020
#define VIDEO_RESO_VGA		0x0040
#define VIDEO_RESO_HD720	0x0080
#define VIDEO_RESO_HD1080	0x0100


typedef struct {
	int element_type;
	int identifier;
	int compression;
	int input_number;
	int resolution;
} cap_element;

typedef struct {
	int section_type;
	int section_size;
	int n_elems;
	cap_element* elements;
} cap_section;

typedef struct {
	int version;
	int section_count;
	cap_section* sections;
} cap_list;

int get_capability_list(cap_list* caps);

void log_capabilities(int level, cap_list* caps);

#endif /* CAPS_H_ */
