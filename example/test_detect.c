/*
 * test_detect.c
 *
 *  Created on: Sep 9, 2012
 *      Author: arash
 */

#include <tlog/tlog.h>

#include "device.c"

int main()
{
	rcp_device *devs;
	int num;

	tlog_init(TLOG_MODE_STDERR, TLOG_DEBUG, NULL);

	autodetect(&devs, &num);

	INFO("%d device%s detected", num, num>1?"s":"");
	for (int i=0; i<num; i++)
	{
		log_device(TLOG_INFO, &devs[i]);
		INFO("------------------------");
	}

	free(devs);


	return 0;
}
