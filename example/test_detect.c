/*
 * test_detect.c
 *
 *  Created on: Sep 9, 2012
 *      Author: arash
 */

#include "device.c"

int main()
{
	rcp_device *devs;
	int num;

	rcplog_init(LOG_MODE_STDERR, RCP_LOG_DEBUG, NULL);

	autodetect(&devs, &num);

	INFO("%d device%s detected", num, num>1?"s":"");
	for (int i=0; i<num; i++)
	{
		log_device(RCP_LOG_INFO, &devs[i]);
		INFO("------------------------");
	}

	free(devs);


	return 0;
}
