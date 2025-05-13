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

#include <dispatch/dispatch.h>
#include <stdio.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <tlog/tlog.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

#include "rcpcommand.h"
#include "rcpdefs.h"
#include "rcpplus.h"

#define MAX_PASSPHRASE_LEN	1000

rcp_packet response[256];
dispatch_semaphore_t resp_available[256];

#define MAX_ALARM_TAG	0x0bff

event_handler_t registered_events[MAX_ALARM_TAG];
void* registered_events_param[MAX_ALARM_TAG];

pthread_t event_handler_thread;
pthread_mutex_t send_mutex;

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

	pthread_mutex_lock(&send_mutex);
	const ssize_t res = send(con.control_socket, buffer, len, 0);
	pthread_mutex_unlock(&send_mutex);

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
	unsigned char buffer[RCP_MAX_PACKET_LEN];

	ssize_t res = recv(con.control_socket, buffer, TPKT_HEADER_LENGTH, 0);
	if (res == -1)
		goto error;

	int len = ntohs(*(unsigned short*)(buffer + 2)) - TPKT_HEADER_LENGTH;

	ssize_t received = 0;
	while (received < len) {
		res = recv(con.control_socket, buffer + received, len - received, 0);
		if (res == -1)
			goto error;
		TL_DEBUG("%d bytes received", res);
		received += res;
	}

	tlog_hex(TLOG_DEBUG, "received", buffer, received);

	const int request_id = get_request_id(buffer);

	rcp_packet* hdr = &response[request_id];

	read_rcp_header(buffer, hdr);

	memcpy(hdr->payload, buffer+RCP_HEADER_LENGTH, hdr->payload_length);

	return request_id;

error:
	TL_ERROR("rcp_recv: %d - %s\n", errno, strerror(errno));
	return -1;
}

int init_rcp_header(rcp_packet* hdr, const int session_id, const int tag, const int rw, const int data_type)
{
	static int request_id = 1;

	memset(hdr, 0, sizeof(rcp_packet));

	hdr->version = RCP_VERSION;
	hdr->tag = tag;
	hdr->rw = rw;
	hdr->action = RCP_PACKET_ACTION_REQUEST;
	hdr->data_type = data_type;
	hdr->request_id = ++request_id % 0x100;

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
	hdr->action = packet[4] & 0x7F;
	hdr->request_id = packet[5];
	hdr->client_id = ntohs(*(unsigned short*)(packet+6));
	hdr->session_id = ntohl(*(unsigned int*)(packet+8));
	hdr->numeric_descriptor = ntohs(*(unsigned short*)(packet+12));
	hdr->payload_length = ntohs(*(unsigned short*)(packet+14));

	return 0;
}

int wait_with_timeout(const dispatch_semaphore_t sem, const int timeout_ms)
{
	const dispatch_time_t disp_time = dispatch_time(DISPATCH_WALLTIME_NOW,
		timeout_ms * 1000000);
    return dispatch_semaphore_wait(sem, disp_time);
}

rcp_packet* rcp_command(rcp_packet* req)
{
	const int res = rcp_send(req);
	if (res == -1)
		return NULL;

	rcp_packet* resp_packet = &response[req->request_id];

	TL_DEBUG("-> %d 0x%04x", req->request_id, req->tag);

	do {
		if (wait_with_timeout(resp_available[req->request_id], 10000)) {
			TL_ERROR("Timeout on #%d 0x%04x", req->request_id, req->tag);
			return NULL;
		}
		TL_DEBUG("<- %d 0x%04x", resp_packet->request_id, resp_packet->tag);
	} while (req->tag != resp_packet->tag);

	const unsigned char action = resp_packet->action;
	if (action == RCP_PACKET_ACTION_ERROR) {
		unsigned char err_code = resp_packet->payload[0];
		TL_WARNING("Resp. action of 0x%04X is %d: 0x%02x - %s",
			resp_packet->tag, action, err_code, error_str(err_code));
		return NULL;
	}

	return resp_packet;
}

static void* message_manager_routine(void* params)
{
	UNUSED(params);

	while (1) {
		const int request_id = rcp_recv();
		if (request_id != -1) {
			const int tag = response[request_id].tag;
			//TL_INFO("packet received: %d", tag);
			dispatch_semaphore_signal(resp_available[request_id]);

			if (tag < MAX_ALARM_TAG && registered_events[tag] != NULL) {
				//TL_INFO("registered event : ");
				//TL_INFO("%d", tag);
				registered_events[tag](&response[request_id], registered_events_param[tag]);
			}
		}
	}

	return NULL;
}

int register_event(int event_tag, event_handler_t handler, void* param)
{
	registered_events[event_tag] = handler;
	registered_events_param[event_tag] = param;
	return 0;
}

int start_message_manager()
{
	pthread_create(&event_handler_thread, NULL, message_manager_routine, NULL);

	for (int i = 0; i < 256; i++)
		resp_available[i] = dispatch_semaphore_create(0);

	memset(registered_events, 0, sizeof(event_handler_t)*MAX_ALARM_TAG);
	return 0;
}

int stop_message_manager()
{
	pthread_cancel(event_handler_thread);

	for (int i = 0; i < 256; ++i)
		dispatch_release(resp_available[i]);

	return 0;
}

// static void md5(unsigned char* in, int len, unsigned char* digest)
// {
//	MD5_CTX ctx;
//	MD5Init(&ctx);
//	MD5Update(&ctx, in, len);
//	MD5Final(&ctx);
//
//	memcpy(digest, ctx.digest, 16);
// }

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
			//tlog_hex(TLOG_INFO, "md5 random str", &instr[1], 16);

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
	pthread_mutex_init(&send_mutex, NULL);

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
	pthread_mutex_destroy(&send_mutex);

	rcp_packet unreg_req;

	init_rcp_header(&unreg_req, 0, RCP_COMMAND_CONF_RCP_CLIENT_UNREGISTER, RCP_COMMAND_MODE_WRITE, RCP_DATA_TYPE_P_OCTET);

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
	char* mdesc = con_req.payload;

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

	desc->coder = con_resp->payload[16];
	desc->line = con_resp->payload[17];
	desc->coding = ntohs(*(unsigned short*)&con_resp->payload[24]);
	desc->resolution = ntohs(*(unsigned short*)&con_resp->payload[26]);

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
	const int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		TL_ERROR("error opening socket : %d - %s", errno, strerror(errno));
		return -1;
	}

	struct hostent* server = gethostbyname(ip);
	if (server == NULL) {
		TL_ERROR("no such host as %s", ip);
		return -1;
	}

	struct sockaddr_in serveraddr;
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
	int pos = 0;
	ssize_t read_len = recv(sockfd, data + pos, 2048, 0);
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

int request_sps_pps(const rcp_session* session, const int coder, char* data, int* len)
{
	rcp_packet req;
	*len = 0;

	init_rcp_header(&req, session->session_id, RCP_COMMAND_CONF_ENC_GET_SPS_AND_PPS,
		RCP_COMMAND_MODE_READ, RCP_DATA_TYPE_P_OCTET);

	req.numeric_descriptor = coder;

	TL_DEBUG("%d sending 0x%04x request", req.request_id, req.tag);

	rcp_packet* resp = rcp_command(&req);
	if (!resp)
		goto error;

	TL_DEBUG("%d got 0x%04x response", resp->request_id, resp->tag);
	tlog_hex(TLOG_DEBUG, "sps_pps_resp", resp->payload, resp->payload_length);

	memcpy(data, resp->payload, resp->payload_length);
	*len = resp->payload_length;

	return 0;

error:
	TL_ERROR("request_sps_pps()");
	return -1;
}

int set_password(const int level, const char* password)
{
    rcp_packet req;
    init_rcp_header(&req, 0, RCP_COMMAND_CONF_PASSWORD_SETTINGS, RCP_COMMAND_MODE_WRITE, RCP_DATA_TYPE_P_STRING);

    req.numeric_descriptor = level;
    strcpy(req.payload, password);

    rcp_packet* resp = rcp_command(&req);
    if (resp == NULL)
        goto error;

    return 0;

error:
    TL_ERROR("set_password()");
    return -1;
}

int set_defaults()
{
    rcp_packet req;
    init_rcp_header(&req, 0, RCP_COMMAND_CONF_DEFAULTS, RCP_COMMAND_MODE_WRITE, RCP_DATA_TYPE_F_FLAG);

    req.payload[0] = 1;

    rcp_packet* resp = rcp_command(&req);
    if (resp == NULL)
        goto error;

    return 0;

error:
    TL_ERROR("set_defaults()");
    return -1;
}

int set_factory_defaults()
{
    rcp_packet req;
    init_rcp_header(&req, 0, RCP_COMMAND_CONF_FACTORY_DEFAULTS, RCP_COMMAND_MODE_WRITE, RCP_DATA_TYPE_F_FLAG);

    req.payload[0] = 1;

    rcp_packet* resp = rcp_command(&req);
    if (resp == NULL)
        goto error;

    return 0;

error:
    TL_ERROR("set_factory_defaults()");
    return -1;
}

int board_reset()
{
    rcp_packet req;
    init_rcp_header(&req, 0, RCP_COMMAND_CONF_BOARD_RESET, RCP_COMMAND_MODE_WRITE, RCP_DATA_TYPE_F_FLAG);

    req.payload[0] = 1;

    rcp_packet* resp = rcp_command(&req);
    if (resp == NULL)
        goto error;

    return 0;

error:
    TL_ERROR("board_reset()");
    return -1;
}

int get_video_input_format(int line, int* format)
{
    rcp_packet req;
    init_rcp_header(&req, 0, RCP_COMMAND_CONF_VIDEO_INPUT_FORMAT, RCP_COMMAND_MODE_READ, RCP_DATA_TYPE_T_DWORD);

    req.numeric_descriptor = line;

    rcp_packet* resp = rcp_command(&req);
    if (resp == NULL)
        goto error;
    *format = ntohl(*(unsigned int*)resp->payload);

    TL_DEBUG("format = %d", *format);

    return 0;

error:
    TL_ERROR("get_video_input_format()");
    return -1;
}

int get_video_input_format_ex(int line, int* format)
{
    rcp_packet req;
    init_rcp_header(&req, 0, RCP_COMMAND_CONF_VIDEO_INPUT_FORMAT_EX, RCP_COMMAND_MODE_READ, RCP_DATA_TYPE_P_OCTET);

    req.numeric_descriptor = line;

    rcp_packet* resp = rcp_command(&req);
    if (resp == NULL)
        goto error;
    *format = resp->payload[1];

    TL_INFO("format = %d", *format);

    return 0;

error:
    TL_ERROR("get_video_input_format_ex()");
    return -1;
}

int get_video_output_format(int line, int* format)
{
    rcp_packet req;
    init_rcp_header(&req, 0, RCP_COMMAND_CONF_VIDEO_OUT_STANDARD, RCP_COMMAND_MODE_READ, RCP_DATA_TYPE_T_OCTET);

    req.numeric_descriptor = line;

    rcp_packet* resp = rcp_command(&req);
    if (resp == NULL)
        goto error;
    *format = resp->payload[0];

    TL_INFO("format = %d", *format);

    return 0;

error:
    TL_ERROR("get_video_output_format()");
    return -1;
}

int set_video_output_format(int line, int format)
{
    rcp_packet req;
    init_rcp_header(&req, 0, RCP_COMMAND_CONF_VIDEO_OUT_STANDARD, RCP_COMMAND_MODE_WRITE, RCP_DATA_TYPE_T_OCTET);

    req.numeric_descriptor = line;

    req.payload[0] = format;
    req.payload_length = 1;

    rcp_packet* resp = rcp_command(&req);
    if (resp == NULL)
        goto error;

    return 0;

error:
    TL_ERROR("get_video_output_format()");
    return -1;
}

int get_video_input_frequency(int line, int* frequency)
{
    rcp_packet req;
    init_rcp_header(&req, 0, RCP_COMMAND_CONF_VIN_BASE_FRAMERATE, RCP_COMMAND_MODE_READ, RCP_DATA_TYPE_T_DWORD);

    req.numeric_descriptor = line;

    rcp_packet* resp = rcp_command(&req);
    if (resp == NULL)
        goto error;
    *frequency = ntohl(*(unsigned int*)(resp->payload));

    TL_INFO("frequency = %d", *frequency);

    return 0;

error:
    TL_ERROR("get_video_input_frequency()");
	return -1;
}
