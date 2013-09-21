/*
 * test_jpeg.c
 *
 *  Created on: Sep 18, 2013
 *      Author: arash
 */

#include "rcpplus.h"
#include <stdio.h>
#include <sys/time.h>

#include <tlog/tlog.h>

int main()
{
	tlog_init(TLOG_MODE_STDERR, TLOG_INFO, NULL);
	char data[200000];
	int len;

	struct timeval st,end;
	gettimeofday(&st, NULL);
	get_jpeg_snapshot("10.25.25.223", data, &len);
	gettimeofday(&end, NULL);

	printf("%ld\n", (end.tv_usec - st.tv_usec) / 1000);

	FILE* out = fopen("snap.jpeg", "w");
	fwrite(data, len, 1, out);
	fclose(out);

	return 0;
}
