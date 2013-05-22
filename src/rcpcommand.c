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
#include <errno.h>
#include <tlog/tlog.h>

#include "rcpcommand.h"
#include "rcpdefs.h"
#include "rcpplus.h"
#include "md5.h"

#define MAX_PAYLOAD_LENGHT	1000
#define MAX_PASSPHRASE_LEN	1000

static void md5(unsigned char* in, int len, unsigned char* digest)
{
	MD5_CTX ctx;
	MD5Init(&ctx);
	MD5Update(&ctx, in, len);
	MD5Final(&ctx);

	memcpy(digest, ctx.digest, 16);
}

static int generate_passphrase(int mode, int user_level, char* password, char* pphrase, int* len)
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
			ERROR("generate_passphrase() - invalid encryption mode %d", mode);
			return -1;
		break;
	}

	return 0;
}

static int write_TPKT_header(unsigned char* buffer, int len)
{
	buffer[0] = TPKT_VERSION;
	buffer[1] = 0;
	buffer[2] = len / 0x100;
	buffer[3] = len % 0x100;

	return 0;
}

static int rcp_send(rcp_packet* hdr)
{
	unsigned char buffer[RCP_MAX_PACKET_LEN];

	int len = hdr->payload_length + RCP_HEADER_LENGTH + TPKT_HEADER_LENGTH;

	write_TPKT_header(buffer, len);

	write_rcp_header(buffer+TPKT_HEADER_LENGTH, hdr);

	memcpy(buffer + RCP_HEADER_LENGTH + TPKT_HEADER_LENGTH, hdr->payload, hdr->payload_length);

	DEBUG("sending %d bytes...", len);
	log_hex(TLOG_DEBUG, "data", buffer, len);
	int res = send(con.control_socket, buffer, len, 0);
	DEBUG("%d sent", res);
	if (res == -1)
		ERROR("unable to send packet: %d - %s\n", errno, strerror(errno));

	return res;
}

static int rcp_recv(rcp_packet* hdr)
{
	int res;
	int len;
	int received;

	unsigned char buffer[RCP_MAX_PACKET_LEN];

	res = recv(con.control_socket, buffer, TPKT_HEADER_LENGTH, 0);
	if (res == -1)
		goto error;

	len = ntohs(*(unsigned short*)(buffer+2));
	len -= TPKT_HEADER_LENGTH;

	received = 0;
	while (received < len)
	{
		res = recv(con.control_socket, buffer+received, len-received, 0);
		if (res == -1)
			goto error;
		DEBUG("%d bytes received", res);
		received += res;
	}

	log_hex(TLOG_DEBUG, "received", buffer, received);

	read_rcp_header(buffer, hdr);

	memcpy(hdr->payload, buffer+RCP_HEADER_LENGTH, hdr->payload_length);

	if (hdr->action == RCP_PACKET_ACTION_ERROR)
	{
		ERROR(error_str(hdr->payload[0]));
		return -1;
	}

	return len;

error:
	ERROR("rcp_recv: %d - %s\n", errno, strerror(errno));
	return -1;
}


int rcp_command(rcp_packet* req, rcp_packet* rsp)
{
	int res = rcp_send(req);
	if (res == -1)
		goto error;

	do
	{
		res = rcp_recv(rsp);
		if (res == -1)
			goto error;
	}
	while (req->request_id != rsp->request_id);

	return 0;

error:
	ERROR("rcp_command()");
	return -1;
}

int get_md5_random(unsigned char* md5)
{
	rcp_packet md5_req, md5_resp;
	int res;

	init_rcp_header(&md5_req, NULL, RCP_COMMAND_CONF_RCP_REG_MD5_RANDOM, RCP_COMMAND_MODE_READ, RCP_DATA_TYPE_P_STRING);

	res = rcp_command(&md5_req, &md5_resp);
	if (res == -1)
		goto error;

	DEBUG("payload len = %d", md5_resp.payload_length);
	memcpy(md5, md5_resp.payload, md5_resp.payload_length);

	return 0;

error:
	ERROR("get_md5_random()");
	return -1;
}

int client_register(int type, int mode, rcp_session* session)
{
	rcp_packet reg_req, reg_resp;
	int res;

	init_rcp_header(&reg_req, NULL, RCP_COMMAND_CONF_RCP_CLIENT_REGISTRATION, RCP_COMMAND_MODE_WRITE, RCP_DATA_TYPE_P_OCTET);

	char pphrase[MAX_PASSPHRASE_LEN];
	int plen;
	res = generate_passphrase(mode, session->user_level, session->password, pphrase, &plen);
	if (res == -1)
		goto error;

	log_hex(TLOG_DEBUG, "passphrase", pphrase, plen);

	unsigned short tmp16;
	reg_req.payload[0] = type;
	reg_req.payload[1] = 0;
	tmp16 = htons(0);
	memcpy(reg_req.payload+2, &tmp16, 2);
	reg_req.payload[4] = mode;
	reg_req.payload[5] = plen;
	tmp16 = htons(1);
	memcpy(reg_req.payload+6, &tmp16, 2);
	tmp16 = htons(0xffff);
	memcpy(reg_req.payload+8, &tmp16, 2);
	memcpy(reg_req.payload+10, pphrase, plen);
	//cl_reg.payload[plen+10] = 0;
	reg_req.payload_length = plen + 10;

	//log_hex("client register payload", cl_reg.payload, cl_reg.payload_length);

	res = rcp_command(&reg_req, &reg_resp);
	if (res == -1)
		goto error;

	log_hex(TLOG_DEBUG, "client register response", reg_resp.payload, reg_resp.payload_length);
	if (reg_resp.payload[0] == 0)
		goto error;

	session->user_level = reg_resp.payload[1];
	session->client_id = ntohs(*(unsigned short*)(reg_resp.payload+2));

	return 0;

error:
	ERROR("client_register()");
	return -1;
}

static char* connect_stat_str(int stat)
{
	switch (stat)
	{
		case 0:
			return ("Interface not available");

		case 1:
			return ("Access granted");

		case 2:
			return ("Access rejected");

		case 3:
			return ("Session Id invalid or connection no longer active");

		case 4:
			return ("Coding parameter incompatible");

		case 5:
			return ("Interface data will be supplied by another media descriptor");

		case 8:
			return ("The method field in the header does not carry a known method");

		case 9:
			return ("The media field in the header does not carry a known media type");

	}

	return ("Unknown status");
}

int client_connect(rcp_session* session, int method, int media, int flags, rcp_media_descriptor* desc)
{
	rcp_packet con_req, con_resp;
	int res;

	init_rcp_header(&con_req, session, RCP_COMMAND_CONF_CONNECT_PRIMITIVE, RCP_COMMAND_MODE_WRITE, RCP_DATA_TYPE_P_OCTET);

	int len = 0;
	unsigned char* mdesc = con_req.payload;

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

	con_req.payload_length = len;

	res = rcp_command(&con_req, &con_resp);
	if (res == -1)
		goto error;

	INFO("%s", connect_stat_str(con_resp.payload[2]));
	if (con_resp.payload[2] != 1)
		goto error;

	session->session_id = con_resp.session_id;
	DEBUG("session id = %d - %d", con_resp.session_id, session->session_id);
	log_hex(TLOG_INFO, "client connection resp", con_resp.payload, con_resp.payload_length);

	return 0;

error:
	ERROR("client_connect()");
	return -1;
}

int get_capability_list(rcp_session* session)
{
	rcp_packet caps_req, caps_resp;
	int res;

	init_rcp_header(&caps_req, session, RCP_COMMAND_CONF_CAPABILITY_LIST, RCP_COMMAND_MODE_READ, RCP_DATA_TYPE_P_OCTET);

	res = rcp_command(&caps_req, &caps_resp);
	if (res == -1)
		goto error;

	log_hex(TLOG_INFO, "cap list response", caps_resp.payload, caps_resp.payload_length);
	return 0;

error:
	ERROR("capability_list()");
	return -1;
}

int keep_alive(rcp_session* session)
{
	rcp_packet alive_req, alive_resp;
	int res;

	init_rcp_header(&alive_req, session, RCP_COMMAND_CONF_RCP_CONNECTIONS_ALIVE, RCP_COMMAND_MODE_READ, RCP_DATA_TYPE_F_FLAG);

	res = rcp_command(&alive_req, &alive_resp);
	if (res == -1)
		goto error;

	return ntohl(*(unsigned int*)alive_resp.payload);

error:
	ERROR("keep_alive()");
	return -1;
}
