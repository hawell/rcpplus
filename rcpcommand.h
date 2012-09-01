/*
 * rcpcommand.h
 *
 *  Created on: Aug 21, 2012
 *      Author: arash
 */

#ifndef RCPCOMMAND_H_
#define RCPCOMMAND_H_

#include "rcpplus.h"

int get_md5_random(unsigned char* md5);

int client_register(int type, int mode, rcp_session* session);

int client_connect(rcp_session* session, int method, int media, int flags, rcp_media_descriptor* desc);

int get_capability_list(rcp_session* session);

int get_coder_list(rcp_session* session, int coder_type, int media_type, rcp_coder_list* coder_list);

int keep_alive(rcp_session* session);

#endif /* RCPCOMMAND_H_ */
