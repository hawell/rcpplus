/*
 * rcpcommand.cpp
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

#include <stdio.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>

#include "rcpcommand.h"
#include "rcpdefs.h"
#include "rcpplus.h"
#include "md5.h"

#define MAX_PAYLOAD_LENGHT	1000
#define MAX_PASSPHRASE_LEN	1000

void log_hex(const char* str, void* d, int l)
{
	unsigned char* p = (unsigned char*)d;
	fprintf(stderr, "%s: (%d) ", str, l);

	for (int i=0; i<l; i++)
		fprintf(stderr, "%02hhx:", p[i]);
	fprintf(stderr, "\n");
}

void md5(unsigned char* in, int len, unsigned char* digest)
{
	MD5_CTX ctx;
	MD5Init(&ctx);
	MD5Update(&ctx, in, len);
	MD5Final(&ctx);

	memcpy(digest, ctx.digest, 16);
}

int generate_passphrase(int mode, int user_level, char* password, char* pphrase, int* len)
{
	char uname[20];
	switch (user_level)
	{
		case RCP_USER_LEVEL_LIVE:strcpy(uname, "live");break;
		case RCP_USER_LEVEL_SERVICE:strcpy(uname, "service");break;
		case RCP_USER_LEVEL_USER:strcpy(uname, "user");break;
		default:break;
	}

	switch (mode)
	{
		case RCP_ENCRYPTION_MODE_PLAIN:
			sprintf(pphrase, "+%s:%s+", uname, password);
			*len = strlen(pphrase);
		break;

		case RCP_ENCRYPTION_MODE_MD5:
		{
			char instr[1000];

			instr[0] = '+';
			get_md5_random((unsigned char*)&instr[1]);
			//log_hex("md5 random str", &instr[1], 16);

/*
			sprintf(&instr[17], "+++%s:%s+", uname, password);
			unsigned char response[16];
			md5((unsigned char*)instr, strlen(instr), response);
			sprintf(pphrase, "+%s:%16s:%16s+", uname, "", "");
			*len = strlen(uname) + 36;
			for (int i=0; i<16; i++)
			{
				pphrase[*len-17+i] = response[i];
				pphrase[*len-33+i] = instr[1+i];
			}
*/

			unsigned char response[] = {
			        0x30, 0x37, 0x35, 0x64,
			        0x65, 0x30, 0x63, 0x30,
			        0x39, 0x36, 0x66, 0x33,
			        0x34, 0x38, 0x30, 0x31,
			        0x34, 0x61, 0x32, 0x63,
			        0x35, 0x34, 0x30, 0x37,
			        0x35, 0x39, 0x33, 0x38,
			        0x66, 0x31, 0x62, 0x34,
			};
			sprintf(pphrase, "+%s:%16s:%32s+", uname, "", "");
			*len = strlen(uname) + 2;
			for (int i=0; i<16; i++)
				pphrase[*len+i] = instr[1+i];
			*len += 17;
			memcpy(pphrase + *len, response, 32);
			*len += 33;
		}
		break;

		default:
			fprintf(stderr, "ERROR : generate_passphrase() - invalid encryption mode %d\n", mode);
			return -1;
		break;
	}

	return 0;
}

int get_md5_random(unsigned char* md5)
{
	rcp_packet md5_reg;
	init_rcp_header(&md5_reg, RCP_COMMAND_CONF_RCP_REG_MD5_RANDOM);

	int res;

	//fprintf(stderr, "sending md5 reg\n");
	res = rcp_send(&md5_reg);
	if (res == -1)
		goto error;
	//fprintf(stderr, "sent\n");

	rcp_packet md5_resp;
	res = rcp_recv(&md5_resp);
	if (res == -1)
		goto error;
	//fprintf(stderr, "payload len = %d\n", md5_resp.payload_length);
	memcpy(md5, md5_resp.payload, md5_resp.payload_length);

	return 0;

error:
	fprintf(stderr, "ERROR : get_md5_random()\n");
	return -1;
}

int client_register(int type, int mode, rcp_session* session)
{
	int res;
	rcp_packet cl_reg;
	init_rcp_header(&cl_reg, RCP_COMMAND_CONF_RCP_CLIENT_REGISTRATION);

	char pphrase[MAX_PASSPHRASE_LEN];
	int plen;
	generate_passphrase(mode, session->user_level, session->password, pphrase, &plen);
	//log_hex("passphrase", pphrase, plen);

	unsigned short tmp16;
	cl_reg.payload[0] = type;
	cl_reg.payload[1] = 0;
	tmp16 = htons(0);
	memcpy(cl_reg.payload+2, &tmp16, 2);
	cl_reg.payload[4] = mode;
	cl_reg.payload[5] = plen;
	tmp16 = htons(1);
	memcpy(cl_reg.payload+6, &tmp16, 2);
	tmp16 = htons(0xffff);
	memcpy(cl_reg.payload+8, &tmp16, 2);
	memcpy(cl_reg.payload+10, pphrase, plen);
	//cl_reg.payload[plen+10] = 0;
	cl_reg.payload_length = plen + 10;

	//log_hex("client register payload", cl_reg.payload, cl_reg.payload_length);

	res = rcp_send(&cl_reg);
	if (res == -1)
		goto error;

	rcp_packet reg_resp;
	res = rcp_recv(&reg_resp);
	if (res == -1)
		goto error;

	log_hex("client register response", reg_resp.payload, reg_resp.payload_length);
	session->client_id = ntohs(*(unsigned short*)(reg_resp.payload+2));

	return 0;

error:
	fprintf(stderr, "ERROR : client_register()\n");
	return -1;
}

int client_connect(rcp_session* session, int method, int media, int flags, rcp_media_descriptor* desc)
{
	rcp_packet cl_con;
	init_rcp_header(&cl_con, RCP_COMMAND_CONF_CONNECT_PRIMITIVE);
	cl_con.client_id = session->client_id;

	int len = 0;
	unsigned char* mdesc = cl_con.payload;
	while (desc->encapsulation_protocol != 0)
	{
		unsigned short tmp16;
		mdesc[0] = method;
		mdesc[1] = media;
		mdesc[2] = 0;
		mdesc[3] = flags;
		mdesc[4] = mdesc[5] = mdesc[6] = mdesc[7] = 0;

		mdesc[8] = desc->encapsulation_protocol;
		mdesc[9] = desc->substitude_connection | (desc->relative_addressing << 1);
		tmp16 = htons(desc->mta_port);
		memcpy(mdesc+10, &tmp16, 2);
		memcpy(mdesc+12, &desc->mta_ip, 4);
		mdesc[16] = desc->coder;
		mdesc[17] = desc->line;
		tmp16 = htons(desc->coding);
		memcpy(mdesc+24, &tmp16, 2);
		tmp16 = htons(desc->resolution);
		memcpy(mdesc+26, &tmp16, 2);
		mdesc[28] = 0;
		mdesc[29] = 0;
		tmp16 = htons(1000);
		memcpy(mdesc+30, &tmp16, 2);

		mdesc += 32;
		len += 32;
		desc++;
	}

	cl_con.payload_length = len;

	rcp_send(&cl_con);

	rcp_packet con_resp;
	rcp_recv(&con_resp);
	session->session_id = con_resp.session_id;
	fprintf(stderr, "$$$$$$$$$$$$$$$$$$$$$$$$$$$ session id = %d - %d\n", con_resp.session_id, session->session_id);
	log_hex("client connection resp", con_resp.payload, con_resp.payload_length);

	return 0;
}

int get_capability_list(rcp_session* session)
{
	rcp_packet caps;
	int res;

	init_rcp_header(&caps, RCP_COMMAND_CONF_CAPABILITY_LIST);
	caps.client_id = session->client_id;

	res = rcp_send(&caps);
	if (res == -1)
		goto error;

	rcp_packet caps_resp;
	res = rcp_recv(&caps_resp);
	if (res == -1)
		goto error;

	log_hex("cap list response", caps_resp.payload, caps_resp.payload_length);
	return 0;

error:
	fprintf(stderr, "ERROR : capability_list()\n");
	return -1;
}

#define CODER_SIZE_IN_PAYLOAD	16

int get_coder_list(rcp_session* session, int coder_type, int media_type, rcp_coder_list* coder_list)
{
	rcp_packet coders_req;
	int res;

	init_rcp_header(&coders_req, RCP_COMMAND_CONF_RCP_CODER_LIST);
	coders_req.numeric_descriptor = 1; // line number - where do we get this?!!
	coders_req.client_id = session->client_id;

	coders_req.payload[0] = RCP_MEDIA_TYPE_VIDEO;
	coders_req.payload[1] = coder_type;
	coders_req.payload[2] = 1;
	coders_req.payload_length = 3;

	res = rcp_send(&coders_req);
	if (res == -1)
		goto error;

	rcp_packet coders_resp;
	res = rcp_recv(&coders_resp);
	if (res == -1)
		goto error;

	coder_list->count = coders_resp.payload_length / CODER_SIZE_IN_PAYLOAD;
	coder_list->coder = (rcp_coder*)malloc(sizeof(rcp_coder) * coder_list->count);

	unsigned char* pos = coders_resp.payload;
	for (int i=0; i<coder_list->count; i++)
	{
		rcp_coder* c = coder_list->coder + i;
		c->direction = coder_type;
		c->media_type = media_type;
		c->number = ntohs(*(unsigned short*)(pos));
		c->caps = ntohs(*(unsigned short*)(pos + 2));
		c->current_cap = ntohs(*(unsigned short*)(pos + 4));
		c->param_caps = ntohs(*(unsigned short*)(pos + 6));
		c->current_param = ntohs(*(unsigned short*)(pos + 8));
		pos += CODER_SIZE_IN_PAYLOAD;
	}

	return 0;

error:
	fprintf(stderr, "ERROR : coder_list()\n");
	return -1;
}

int keep_alive(rcp_session* session)
{
	rcp_packet alive_req;
	int res;

	init_rcp_header(&alive_req, RCP_COMMAND_CONF_RCP_CONNECTIONS_ALIVE);
	alive_req.client_id = session->client_id;
	alive_req.session_id = session->session_id;

	res = rcp_send(&alive_req);
	if (res == -1)
		goto error;

	rcp_packet alive_resp;
	res = rcp_recv(&alive_resp);
	if (res == -1)
		goto error;

	return alive_resp.payload[0];

error:
	fprintf(stderr, "ERROR : keep_alive()\n");
	return -1;
}
