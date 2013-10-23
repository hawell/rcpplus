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
#include <netdb.h>
#include <unistd.h>
#include <time.h>
#include <limits.h>
#include <tlog/tlog.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>


#include "rcpcommand.h"
#include "rcpdefs.h"
#include "rcpplus.h"
#include "md5.h"

#define MAX_PAYLOAD_LENGHT	1000
#define MAX_PASSPHRASE_LEN	1000

rcp_packet resp[256];
sem_t resp_available[256];

pthread_t event_handler_thread;

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

	TL_DEBUG("sending %d bytes...", len);
	tlog_hex(TLOG_DEBUG, "data", buffer, len);
	int res = send(con.control_socket, buffer, len, 0);
	TL_DEBUG("%d sent", res);
	if (res == -1)
		TL_ERROR("unable to send packet: %d - %s", errno, strerror(errno));

	return res;
}

static int get_request_id(unsigned char* buffer)
{
	return buffer[5];
}

static int rcp_recv()
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
		TL_DEBUG("%d bytes received", res);
		received += res;
	}

	tlog_hex(TLOG_DEBUG, "received", buffer, received);

	int request_id = get_request_id(buffer);

	rcp_packet* hdr = &resp[request_id];

	read_rcp_header(buffer, hdr);

	memcpy(hdr->payload, buffer+RCP_HEADER_LENGTH, hdr->payload_length);

	if (hdr->action == RCP_PACKET_ACTION_ERROR)
	{
		TL_ERROR(error_str(hdr->payload[0]));
	}

	return request_id;

error:
	TL_ERROR("rcp_recv: %d - %s\n", errno, strerror(errno));
	return -1;
}

int init_rcp_header(rcp_packet* hdr, int session_id, int tag, int rw, int data_type)
{
	memset(hdr, 0, sizeof(rcp_packet));

	hdr->version = RCP_VERSION;
	hdr->tag = tag;
	hdr->rw = rw;
	hdr->action = RCP_PACKET_ACTION_REQUEST;
	hdr->data_type = data_type;
	hdr->request_id = rand() % 0x100;

	hdr->client_id = con.client_id;
	hdr->session_id = session_id;

	return 0;
}

int write_rcp_header(unsigned char* packet, rcp_packet* hdr)
{
	unsigned short tmp16;
	unsigned int tmp32;

	tmp16 = htons(hdr->tag);
	memcpy(packet, &tmp16, 2);

	packet[2] = hdr->data_type;

	packet[3] = (hdr->version << 4) + (hdr->rw);

	packet[4] = (hdr->continuation << 7) + (hdr->action << 4);

	packet[5] = hdr->request_id;

	tmp16 = htons(hdr->client_id);
	memcpy(&packet[6], &tmp16, 2);

	tmp32 = htonl(hdr->session_id);
	memcpy(&packet[8], &tmp32, 4);

	tmp16 = htons(hdr->numeric_descriptor);
	memcpy(&packet[12], &tmp16, 2);

	tmp16 = htons(hdr->payload_length);
	memcpy(&packet[14], &tmp16, 2);

	return 0;
}

int read_rcp_header(unsigned char* packet, rcp_packet* hdr)
{
	hdr->tag = ntohs(*(unsigned short*)packet);
	hdr->data_type = packet[2];
	hdr->version = packet[3] >> 4;
	hdr->rw = packet[3] & 0xF0;
	hdr->continuation = packet[4] >> 7;
	hdr->action = packet[4] & 0x80;
	hdr->request_id = packet[5];
	hdr->client_id = ntohs(*(unsigned short*)(packet+6));
	hdr->session_id = ntohl(*(unsigned int*)(packet+8));
	hdr->numeric_descriptor = ntohs(*(unsigned short*)(packet+12));
	hdr->payload_length = ntohs(*(unsigned short*)(packet+14));

	return 0;
}

rcp_packet* rcp_command(rcp_packet* req)
{
	int res = rcp_send(req);
	if (res == -1)
		goto error;

	sem_wait(&resp_available[req->request_id]);

	return &resp[req->request_id];

error:
	TL_ERROR("rcp_command()");
	return NULL;
}

static void* event_handler(void* params)
{
	while (1)
	{
		int request_id = rcp_recv();
		if (request_id != -1)
		{
			TL_DEBUG("packet received: %d", resp[request_id].tag);
			sem_post(&resp_available[request_id]);
		}
	}

	return NULL;
}

int start_event_handler()
{
	pthread_create(&event_handler_thread, NULL, event_handler, NULL);
	for (int i=0; i<265; i++)
	{
		sem_init(&resp_available[i], 0, 0);
	}
	return 0;
}

int stop_event_handler()
{
	pthread_cancel(event_handler_thread);
	for (int i=0; i<265; i++)
	{
		sem_destroy(&resp_available[i]);
	}
	return 0;
}

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
			//tlog_hex("md5 random str", &instr[1], 16);

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
			TL_ERROR("generate_passphrase() - invalid encryption mode %d", mode);
			return -1;
		break;
	}

	return 0;
}

int get_md5_random(unsigned char* md5)
{
	rcp_packet md5_req;

	init_rcp_header(&md5_req, 0, RCP_COMMAND_CONF_RCP_REG_MD5_RANDOM, RCP_COMMAND_MODE_READ, RCP_DATA_TYPE_P_STRING);

	rcp_packet* md5_resp = rcp_command(&md5_req);
	if (md5_resp == NULL)
		goto error;

	TL_DEBUG("payload len = %d", md5_resp->payload_length);
	memcpy(md5, md5_resp->payload, md5_resp->payload_length);

	return 0;

error:
	TL_ERROR("get_md5_random()");
	return -1;
}

int client_register(int user_level, const char* password, int type, int mode)
{
	con.user_level = user_level;
	strcpy(con.password, password);

	rcp_packet reg_req;

	init_rcp_header(&reg_req, 0, RCP_COMMAND_CONF_RCP_CLIENT_REGISTRATION, RCP_COMMAND_MODE_WRITE, RCP_DATA_TYPE_P_OCTET);

	char pphrase[MAX_PASSPHRASE_LEN];
	int plen;
	int res = generate_passphrase(mode, con.user_level, con.password, pphrase, &plen);
	if (res == -1)
		goto error;

	tlog_hex(TLOG_DEBUG, "passphrase", pphrase, plen);

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

	//tlog_hex("client register payload", cl_reg.payload, cl_reg.payload_length);

	rcp_packet* reg_resp = rcp_command(&reg_req);
	if (reg_resp == NULL)
		goto error;

	tlog_hex(TLOG_DEBUG, "client register response", reg_resp->payload, reg_resp->payload_length);
	if (reg_resp->payload[0] == 0)
		goto error;

	con.user_level = reg_resp->payload[1];
	con.client_id = ntohs(*(unsigned short*)(reg_resp->payload+2));

	return 0;

error:
	TL_ERROR("client_register()");
	return -1;
}

int client_unregister()
{
	rcp_packet unreg_req;

	init_rcp_header(&unreg_req, 0, RCP_COMMAND_CONF_RCP_CLIENT_UNREGISTER, RCP_COMMAND_MODE_WRITE, RCP_DATA_TYPE_T_WORD);

	rcp_packet* unreg_resp = rcp_command(&unreg_req);
	if (unreg_resp == NULL)
		goto error;

	int unregister_result = unreg_resp->payload[0];
	if (unregister_result != 1)
	{
		TL_ERROR("unregister not successful");
		return -1;
	}
	else
		return 0;

error:
	TL_ERROR("client_unregister()");
	return -1;
}

int read_client_registration()
{
	rcp_packet reg_req;

	init_rcp_header(&reg_req, 0, RCP_COMMAND_CONF_RCP_CLIENT_REGISTRATION, RCP_COMMAND_MODE_READ, RCP_DATA_TYPE_P_OCTET);

	rcp_packet* reg_resp = rcp_command(&reg_req);
	if (reg_resp == NULL)
		goto error;

	return 0;

error:
	TL_ERROR("read_client_registration()");
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
	rcp_packet con_req;

	init_rcp_header(&con_req, 0, RCP_COMMAND_CONF_CONNECT_PRIMITIVE, RCP_COMMAND_MODE_WRITE, RCP_DATA_TYPE_P_OCTET);

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

	rcp_packet* con_resp = rcp_command(&con_req);
	if (con_resp == NULL)
		goto error;

	TL_INFO("%s", connect_stat_str(con_resp->payload[2]));
	if (con_resp->payload[2] != 1)
		goto error;

	session->session_id = con_resp->session_id;
	TL_DEBUG("session id = %d - %d", con_resp->session_id, session->session_id);
	tlog_hex(TLOG_DEBUG, "client connection resp", con_resp->payload, con_resp->payload_length);

	return 0;

error:
	TL_ERROR("client_connect()");
	return -1;
}

int client_disconnect(rcp_session* session)
{
	rcp_packet discon_req;

	init_rcp_header(&discon_req, session->session_id, RCP_COMMAND_CONF_DISCONNECT_PRIMITIVE, RCP_COMMAND_MODE_WRITE, RCP_DATA_TYPE_P_OCTET);

	discon_req.payload_length = 8;

	discon_req.payload[0] = 0x01; // connection disconnected
	discon_req.payload[1] = 0x01; // normal termination

	memcpy(discon_req.payload + 4, &con.ctrl_addr.sin_addr.s_addr, 4);

	rcp_packet* discon_resp = rcp_command(&discon_req);
	if (discon_resp == NULL)
		goto error;

	tlog_hex(TLOG_DEBUG, "client disconnect response", discon_resp->payload, discon_resp->payload_length);

	return 0;

error:
	TL_ERROR("client_disconnect()");
	return -1;
}

int get_capability_list()
{
	rcp_packet caps_req;

	init_rcp_header(&caps_req, 0, RCP_COMMAND_CONF_CAPABILITY_LIST, RCP_COMMAND_MODE_READ, RCP_DATA_TYPE_P_OCTET);

	rcp_packet* caps_resp = rcp_command(&caps_req);
	if (caps_resp == NULL)
		goto error;

	tlog_hex(TLOG_INFO, "cap list response", caps_resp->payload, caps_resp->payload_length);
	return 0;

error:
	TL_ERROR("capability_list()");
	return -1;
}

int keep_alive(rcp_session* session)
{
	rcp_packet alive_req;

	init_rcp_header(&alive_req, session->session_id, RCP_COMMAND_CONF_RCP_CONNECTIONS_ALIVE, RCP_COMMAND_MODE_READ, RCP_DATA_TYPE_F_FLAG);

	rcp_packet* alive_resp = rcp_command(&alive_req);
	if (alive_resp == NULL)
		goto error;

	return ntohl(*(unsigned int*)alive_resp->payload);

error:
	TL_ERROR("keep_alive()");
	return -1;
}

int get_jpeg_snapshot(char* ip, char* data, int* len)
{
	int sockfd;
	struct hostent *server;
	struct sockaddr_in serveraddr;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
	{
		TL_ERROR("error opening socket : %d - %s", errno, strerror(errno));
		return -1;
	}
	server = gethostbyname(ip);
	if (server == NULL)
	{
		TL_ERROR("no such host as %s", ip);
		return -1;
	}
	memset((char *) &serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	memcpy((char *)&serveraddr.sin_addr.s_addr, (char *)server->h_addr_list[0], server->h_length);
	serveraddr.sin_port = htons(80);
	if (connect(sockfd, (const struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0)
	{
	      TL_ERROR("error connecting : %d - %s", errno, strerror(errno));
	      return -1;
	}
	const char request_template[] = "GET /snap.jpg HTTP/1.1\r\nHost: %s\r\nUser-Agent: Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:20.0) Gecko/20100101 Firefox/20.0\r\nAccept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\nAccept-Language: en-US,en;q=0.5\r\nAccept-Encoding: gzip, deflate\r\nCookie: VideoInput=1; VidType=MPEG4; Instance=1; gui=1; IsVideo1=1; IsVideo2=1; HcsoB=3f7154f44e371053; leasetimeid=1011325756\r\nConnection: keep-alive\r\n\r\n";
	char request[500];
	sprintf(request, request_template, ip);

	send(sockfd, request, strlen(request), 0);
	int read_len;
	int pos = 0;
	read_len = recv(sockfd, data+pos, 2048, 0);
	int jpeg_size;
	char* _pos = strstr(data, "Content-Length");
	sscanf(_pos + strlen("Content-Length: "), "%d", &jpeg_size);
	_pos = strstr(data, "\r\n\r\n");
	int to_read = jpeg_size - (read_len - ((_pos-data)+4));
	while (to_read)
	{
		read_len = recv(sockfd, data+pos, 4096, 0);
		pos += read_len;
		to_read -= read_len;
	}
	*len = pos;

	return 0;
}

int request_intraframe(rcp_session* session)
{
	rcp_packet req;

	init_rcp_header(&req, session->session_id, RCP_COMMAND_CONF_MPEG4_INTRA_FRAME_REQUEST, RCP_COMMAND_MODE_WRITE, RCP_DATA_TYPE_T_DWORD);

	memcpy(req.payload, (char*)&session->session_id, 4);

	rcp_packet* resp = rcp_command(&req);
	if (resp == NULL)
		goto error;

	return 0;

error:
	TL_ERROR("request_intraframe()");
	return -1;
}

int request_sps_pps(rcp_session* session, int coder, unsigned char* data, int *len)
{
	rcp_packet req;
	*len = 0;

	init_rcp_header(&req, session->session_id, RCP_COMMAND_CONF_ENC_GET_SPS_AND_PPS, RCP_COMMAND_MODE_READ, RCP_DATA_TYPE_P_OCTET);

	req.numeric_descriptor = coder;

	rcp_packet* resp = rcp_command(&req);
	if (resp == NULL)
		goto error;

	TL_INFO("resp action : %d", resp->action);
	tlog_hex(TLOG_DEBUG, "sps_pps_resp", resp->payload, resp->payload_length);

	memcpy(data, resp->payload, resp->payload_length);
	*len = resp->payload_length;

	return 0;

error:
	TL_ERROR("request_sps_pps()");
	return -1;
}
