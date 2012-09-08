/*
 * rcplog.h
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

#ifndef RCPLOG_H_
#define RCPLOG_H_

#include <syslog.h>
#include <stdarg.h>

#define LOG_MODE_STDERR		0
#define LOG_MODE_STDOUT		1
#define LOG_MODE_SYSLOG		2
#define LOG_MODE_FILE		3

#define DEBUG(...) rcplog(LOG_DEBUG, __VA_ARGS__)
#define INFO(...) rcplog(LOG_INFO, __VA_ARGS__)
#define WARNING(...) rcplog(LOG_WARNING, __VA_ARGS__)
#define ERROR(...) rcplog(LOG_ERR, __VA_ARGS__)

int rcplog_init(int mode, int level, void* param);

void rcplog(int level, const char* format, ...);

void log_hex(int level, const char* str, void* d, int l);

const char* error_str(int error_code);


#endif /* RCPLOG_H_ */
