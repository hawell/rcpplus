/*
 * rcpplus.cpp
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
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <netdb.h>
#include <errno.h>
#include <unistd.h>
#include <tlog/tlog.h>

#include "rcpplus.h"
#include "rcpdefs.h"
#include "coder.h"

rcp_connection con;

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

int rcp_connect(const char* ip)
{
	con.control_socket = socket(PF_INET, SOCK_STREAM, 0);
	if (con.control_socket == -1)
	{
		ERROR("cannot open socket : %d - %s", errno, strerror(errno));
		return -1;
	}

	struct hostent *hp;
	hp = gethostbyname(ip);

	memset(&con.ctrl_addr, 0, sizeof(struct sockaddr_in));

	con.ctrl_addr.sin_family = AF_INET;
	memcpy((char *)&con.ctrl_addr.sin_addr, hp->h_addr_list[0],  hp->h_length);
	con.ctrl_addr.sin_port = htons(RCP_CONTROL_PORT);

	int res = connect(con.control_socket, (struct sockaddr *)&con.ctrl_addr, sizeof (con.ctrl_addr));
	if (res == -1)
	{
		ERROR("connection failed : %d - %s\n", errno, strerror(errno));
	}

	return 0;
}

int stream_connect_udp(rcp_session* session)
{
	session->stream_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	session->stream_addr.sin_family = AF_INET;
	session->stream_addr.sin_port = htons(0);
	session->stream_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	int res = bind(session->stream_socket, (struct sockaddr*)&session->stream_addr, sizeof(session->stream_addr));
	if (res == -1)
	{
		ERROR("cannot bind %d - %s", errno, strerror(errno));
		return -1;
	}

	listen(session->stream_socket, 10);

	struct sockaddr_in addr;
	socklen_t len = sizeof(addr);
	getsockname(session->stream_socket, (struct sockaddr*)&addr, &len);

	return ntohs(addr.sin_port);
}

int stream_connect_tcp(rcp_session* session)
{
	session->stream_socket = socket(AF_INET, SOCK_STREAM, 0);

	session->stream_addr.sin_family = AF_INET;
	session->stream_addr.sin_port = htons(0);
	session->stream_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	int res = bind(session->stream_socket, (struct sockaddr*)&session->stream_addr, sizeof(session->stream_addr));
	if (res == -1)
	{
		ERROR("cannot bind %d - %s", errno, strerror(errno));
		return -1;
	}

	res = connect(session->stream_socket, (struct sockaddr*)&session->tcp_stream_addr, sizeof(session->tcp_stream_addr));
	if (res == -1)
	{
		ERROR("cannot connect: %d - %s", errno, strerror(errno));
		return -1;
	}

	struct sockaddr_in addr;
	socklen_t len = sizeof(addr);
	getsockname(session->stream_socket, (struct sockaddr*)&addr, &len);


	return ntohs(addr.sin_port);
}

int initiate_tcp_stream(rcp_session* session, struct rcp_coder_tag* coder)
{
	unsigned char buffer[RCP_MAX_PACKET_LEN];

	int size = sprintf((char*)buffer, "GET /media_tunnel/%08u/%d/%d/%d/%d HTTP 1.0\r\n\r\n", session->session_id, coder->media_type, coder->direction, 1, coder->number);
	int res = send(session->stream_socket, buffer, size, 0);
	if (res == -1)
	{
		ERROR("cannot sent initiative sequence: %d - %s", errno, strerror(errno));
		return -1;
	}

	return 0;
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
	return "";
}
