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

#include <sys/time.h>
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

int rcp_connect(const char* ip)
{
	con.control_socket = socket(PF_INET, SOCK_STREAM, 0);
	if (con.control_socket == -1)
	{
		TL_ERROR("cannot open socket : %d - %s", errno, strerror(errno));
		return -1;
	}

	strcpy(con.address, ip);
	struct hostent* hp = gethostbyname(ip);

	memset(&con.ctrl_addr, 0, sizeof(struct sockaddr_in));

	con.ctrl_addr.sin_family = AF_INET;
	memcpy((char *)&con.ctrl_addr.sin_addr, hp->h_addr_list[0], hp->h_length);
	con.ctrl_addr.sin_port = htons(RCP_CONTROL_PORT);

	const int res = connect(con.control_socket, (struct sockaddr*)&con.ctrl_addr,
		sizeof (con.ctrl_addr));
	if (res == -1) {
		TL_ERROR("connection failed: %d - %s", errno, strerror(errno));
		return res;
	}

	return 0;
}

int rcp_disconnect()
{
	return close(con.control_socket);
}

int stream_connect_udp(rcp_session* session)
{
	session->stream_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	session->stream_addr.sin_family = AF_INET;
	session->stream_addr.sin_port = htons(0);
	session->stream_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	struct timeval tv = {10, 0};
	int res = setsockopt(session->stream_socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
	if (res == -1)
	{
		TL_ERROR("cannot set socket timeout");
		return -1;
	}

	res = bind(session->stream_socket, (struct sockaddr*)&session->stream_addr, sizeof(session->stream_addr));
	if (res == -1)
	{
		TL_ERROR("cannot bind %d - %s", errno, strerror(errno));
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
	session->stream_addr.sin_port = htons(80);

	struct hostent* hp = gethostbyname(con.address);
	memcpy((char *)&session->stream_addr.sin_addr, hp->h_addr_list[0], hp->h_length);

	int res = connect(session->stream_socket, (struct sockaddr*)&session->stream_addr, sizeof(session->stream_addr));
	if (res == -1)
	{
		TL_ERROR("cannot connect: %d - %s", errno, strerror(errno));
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

	const int size = sprintf((char*)buffer, "GET /media_tunnel/%08u/%d/%d/%d/%d HTTP 1.0\r\n\r\n",
		ntohl(session->session_id), coder->media_type, coder->direction, 1, coder->number);
	ssize_t res = send(session->stream_socket, buffer, size, 0);
	if (res == -1) {
		TL_ERROR("cannot sent initiative sequence: %d - %s", errno, strerror(errno));
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
