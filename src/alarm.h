/*
 * alarm.h
 *
 *  Created on: Nov 24, 2015
 *      Author: arash
 */

#ifndef ALARM_H_
#define ALARM_H_

#define RCP_COMMAND_CONF_ENABLE_VCA 			0x0813
#define RCP_COMMAND_CONF_VIPROC_ID				0x0803
#define RCP_COMMAND_CONF_VIPROC_MODE			0x0a65
#define RCP_COMMAND_CONF_VIPROC_ALARM			0x0807
#define RCP_COMMAND_CONF_VIPROC_ALARM_MASK		0x0a23
#define RCP_COMMAND_CONF_VIPROC_SENSITIVE_AREA	0x0b78
#define RCP_COMMAND_CONF_START_VIPROC_CONFIG_EDITING	0x0a38
#define RCP_COMMAND_CONF_STOP_VIPROC_CONFIG_EDITING	0x0a39

#define VIPROC_MODE_SILENT_MOTION	0
#define VIPROC_MODE_MANUAL(d)		(d)
#define VIPROC_MODE_OFF			0xfd
#define VIPROC_MODE_SCHEDULER		0xfe
#define VIPROC_MODE_SCRIPT			0xff

#define VIPROC_ALARM_MOTION			0x8000
#define VIPROC_ALARM_GLOBAL_CHANGE		0x4000
#define VIPROC_ALARM_SIGNAL_BRIGHT		0x2000
#define VIPROC_ALARM_SIGNAL_DARK		0x1000
#define VIPROC_ALARM_SIGNAL_NOISY		0x0800
#define VIPROC_ALARM_IMAGE_BLURRY		0x0400
#define VIPROC_ALARM_SIGNAL_LOSS		0x0200
#define VIPROC_ALARM_REF_FAILED		0x0100		// reference image check failed flag
#define VIPROC_ALARM_INVALID_CONFIG	0x0080

#define BITMASK_LENGHT	3000

typedef struct {
	int width;
	int height;
	int cells_x;
	int cells_y;
	int steps_x;
	int steps_y;
	int start_x;
	int start_y;
	char bitmask[BITMASK_LENGHT];
} image_grid;

unsigned int get_viproc_id(int line);

int get_vca_status(int line);
int set_vca_status(int line, int status);

int get_viproc_mode(int line);
int set_viproc_mode(int line, int mode);

unsigned int get_viproc_alarm_mask(int line);
int set_viproc_alarm_mask(int line, unsigned int* mask);

int viproc_start_edit(int line, unsigned int id);
int viproc_stop_edit(int line, unsigned int id);

int get_viproc_sensitive_area(int line, image_grid* grid);
int set_viproc_sensitive_area(int line, image_grid* grid);

#endif /* ALARM_H_ */
