/*
 * rcpcommand.h
 *
 *  Created on: Aug 21, 2012
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

#ifndef RCPCOMMAND_H_
#define RCPCOMMAND_H_

#include "rcpplus.h"

int rcp_command(rcp_packet* req, rcp_packet* rsp);

int get_md5_random(unsigned char* md5);

int client_register(int user_level, const char* password, int type, int mode);
int client_unregister();

int client_connect(rcp_session* session, int method, int media, int flags, rcp_media_descriptor* desc);
int client_disconnect(rcp_session* session);

int get_capability_list();

int keep_alive(rcp_session* session);

int request_intraframe(rcp_session* session);

#endif /* RCPCOMMAND_H_ */
