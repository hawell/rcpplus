/*
 * rcplog.c
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

#include <stdio.h>
#include <syslog.h>

#include "rcplog.h"
#include "rcpdefs.h"

static int log_mode = LOG_MODE_STDERR;
static int min_level = LOG_WARNING;
static FILE* log_file;
static char log_str[2048];
static const char log_prestr[] = "rcpplus";
static const char level_str[][10] = {"[EMERG]", "[ALERT]", "[CRIT]", "[ERR]", "[WARN]", "[NOTICE]", "[INFO]", "[DEBUG]"};

int rcplog_init(int mode, int level, void* param)
{
	log_mode = mode;
	min_level = level;

	switch (mode)
	{
		case LOG_MODE_FILE:
		{
			log_file = fopen((const char*)param, "w");
		}
		break;

		case LOG_MODE_SYSLOG:
		{
			setlogmask(LOG_UPTO(level));

			openlog(log_prestr, LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
		}
		break;

		case LOG_MODE_STDOUT:
			log_file = stdout;
		break;

		case LOG_MODE_STDERR:
			log_file = stderr;
		break;
	}

	return 0;
}

void rcplog(int level, const char* format, ...)
{
	if (level > min_level)
		return;

	va_list args;

	va_start(args, format);
	vsprintf(log_str, format, args);

	if (log_mode == LOG_MODE_SYSLOG)
		syslog(level, "%s %s", level_str[level], log_str);
	else
	{
		fprintf(log_file, "%s ", level_str[level]);
		fputs(log_str, log_file);
		fputs("\n", log_file);
	}

	va_end(args);
}

void log_hex(int level, const char* str, void* d, int l)
{
	if (level > min_level)
		return;

	unsigned char* p = (unsigned char*)d;
	int pos = sprintf(log_str, "%s: (%d) ", str, l);

	for (int i=0; i<l; i++)
		pos += sprintf(log_str+pos, "%02hhx:", p[i]);

	if (log_mode == LOG_MODE_SYSLOG)
		syslog(level, "%s %s",level_str[level], log_str);
	else
	{
		fprintf(log_file, "%s ", level_str[level]);
		fputs(log_str, log_file);
		fputs("\n", log_file);
	}
}

const char* error_str(int error_code)
{
	switch (error_code)
	{
		case RCP_ERROR_INVALID_VERSION:return "invalid version";break;
		case RCP_ERROR_NOT_REGISTERED:return "not registered";break;
		case RCP_ERROR_INVALID_CLIENT_ID:return "invalid client id";break;
		case RCP_ERROR_INVALID_METHOD:return "invalid method";break;
		case RCP_ERROR_INVALID_CMD:return "invalid command";break;
		case RCP_ERROR_INVALID_ACCESS_TYPE:return "invalid access";break;
		case RCP_ERROR_INVALID_DATA_TYPE:return "invalid data type";break;
		case RCP_ERROR_WRITE_ERROR:return "write error";break;
		case RCP_ERROR_PACKET_SIZE:return "invalid packet size";break;
		case RCP_ERROR_READ_NOT_SUPPORTED:return "read not supported";break;
		case RCP_ERROR_INVALID_AUTH_LEVEL:return "invalid authentication level";break;
		case RCP_ERROR_INVAILD_SESSION_ID:return "invalid session id";break;
		case RCP_ERROR_TRY_LATER:return "try later";break;
		case RCP_ERROR_COMMAND_SPECIFIC:return "command specific error";break;
		case RCP_ERROR_UNKNOWN:
		default:
			return "unknown error";break;
	}
}
