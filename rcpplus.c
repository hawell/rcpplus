/*
 * rcpplus.cpp
 *
 *  Created on: Aug 21, 2012
 *      Author: arash
 */

#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <netdb.h>
#include <errno.h>
#include <unistd.h>

#include "rcpplus.h"
#include "rcpdefs.h"

rcp_connection con;

unsigned char buffer[RCP_MAX_PACKET_LEN];

int init_rcp_header(rcp_packet* hdr, int tag)
{
	memset(hdr, 0, sizeof(rcp_packet));

	hdr->version = RCP_VERSION;
	hdr->tag = tag;

	switch (tag)
	{
		case RCP_COMMAND_CONF_RCP_REG_MD5_RANDOM:
		{
			hdr->data_type = RCP_DATA_TYPE_P_STRING;
			hdr->continuation = 0;
			hdr->action = RCP_PACKET_ACTION_REQUEST;
			hdr->rw = RCP_COMMAND_MODE_READ;
		}
		break;

		case RCP_COMMAND_CONF_RCP_CLIENT_REGISTRATION:
		{
			hdr->data_type = RCP_DATA_TYPE_P_OCTET;
			hdr->continuation = 0;
			hdr->action = RCP_PACKET_ACTION_REQUEST;
			hdr->rw = RCP_COMMAND_MODE_WRITE;
		}
		break;

		case RCP_COMMAND_CONF_CONNECT_PRIMITIVE:
		{
			hdr->data_type = RCP_DATA_TYPE_P_OCTET;
			hdr->continuation = 0;
			hdr->action = RCP_PACKET_ACTION_REQUEST;
			hdr->rw = RCP_COMMAND_MODE_WRITE;
		}
		break;

		case RCP_COMMAND_CONF_CAPABILITY_LIST:
		{
			hdr->data_type = RCP_DATA_TYPE_P_OCTET;
			hdr->continuation = 0;
			hdr->action = RCP_PACKET_ACTION_REQUEST;
			hdr->rw = RCP_COMMAND_MODE_READ;
		}
		break;

		case RCP_COMMAND_CONF_RCP_CODER_LIST:
		{
			hdr->data_type = RCP_DATA_TYPE_P_OCTET;
			hdr->continuation = 0;
			hdr->action = RCP_PACKET_ACTION_REQUEST;
			hdr->rw = RCP_COMMAND_MODE_READ;
		}
		break;

		case RCP_COMMAND_CONF_RCP_CONNECTIONS_ALIVE:
		{
			hdr->data_type = RCP_DATA_TYPE_F_FLAG;
			hdr->continuation = 0;
			hdr->action = RCP_PACKET_ACTION_REQUEST;
			hdr->rw = RCP_COMMAND_MODE_READ;
		}
		break;

		default:
			fprintf(stderr, "header tag not supported : %d\n", tag);
		break;
	}

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
	hdr->client_id = ntohs(*(unsigned short*)(buffer+6));
	hdr->session_id = ntohl(*(unsigned int*)(buffer+8));
	hdr->numeric_descriptor = ntohs(*(unsigned short*)(buffer+12));
	hdr->payload_length = ntohs(*(unsigned short*)(buffer+14));

	return 0;
}

int rcp_connect(char* ip)
{
	con.control_socket = socket(PF_INET, SOCK_STREAM, 0);
	if (con.control_socket == -1)
	{
		fprintf(stderr, "cannot open socket : %d - %s", errno, strerror(errno));
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
		fprintf(stderr, "connection failed : %d - %s\n", errno, strerror(errno));
	}

	return 0;
}

int stream_connect()
{
	con.stream_socket = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);

	con.stream_addr.sin_family = AF_INET;
	con.stream_addr.sin_port = htons(0);
	con.stream_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	int res = bind(con.stream_socket, (struct sockaddr*)&con.stream_addr, sizeof(con.stream_addr));
	if (res == -1)
	{
		fprintf(stderr, "cannot bind %d - %s\n", errno, strerror(errno));
		return -1;
	}

	listen(con.stream_socket, 10);

	struct sockaddr_in addr;
	socklen_t len = sizeof(addr);
	getsockname(con.stream_socket, (struct sockaddr*)&addr, &len);

	return ntohs(addr.sin_port);
}

int write_TPKT_header(unsigned char* buffer, int len)
{
	buffer[0] = TPKT_VERSION;
	buffer[1] = 0;
	buffer[2] = len / 0x100;
	buffer[3] = len % 0x100;

	return 0;
}

int rcp_send(rcp_packet* hdr)
{
	int len = hdr->payload_length + RCP_HEADER_LENGTH + TPKT_HEADER_LENGTH;

	write_TPKT_header(buffer, len);

	write_rcp_header(buffer+TPKT_HEADER_LENGTH, hdr);

	memcpy(buffer + RCP_HEADER_LENGTH + TPKT_HEADER_LENGTH, hdr->payload, hdr->payload_length);

	//fprintf(stderr, "sending %d bytes...\n", len);
	log_hex("data", buffer, len);
	int res = send(con.control_socket, buffer, len, 0);
	//fprintf(stderr, "%d sent\n", res);
	if (res == -1)
		fprintf(stderr, "unable to send packet: %d - %s\n", errno, strerror(errno));

	return res;
}

void error_message(int error_code)
{
	switch (error_code)
	{
		case RCP_ERROR_INVALID_VERSION:fprintf(stderr, "invalid version\n");break;
		case RCP_ERROR_NOT_REGISTERED:fprintf(stderr, "not registered\n");break;
		case RCP_ERROR_INVALID_CLIENT_ID:fprintf(stderr, "invalid client id\n");break;
		case RCP_ERROR_INVALID_METHOD:fprintf(stderr, "invalid method\n");break;
		case RCP_ERROR_INVALID_CMD:fprintf(stderr, "invalid command\n");break;
		case RCP_ERROR_INVALID_ACCESS_TYPE:fprintf(stderr, "invalid access\n");break;
		case RCP_ERROR_INVALID_DATA_TYPE:fprintf(stderr, "invalid data type\n");break;
		case RCP_ERROR_WRITE_ERROR:fprintf(stderr, "write error\n");break;
		case RCP_ERROR_PACKET_SIZE:fprintf(stderr, "invalid packet size\n");break;
		case RCP_ERROR_READ_NOT_SUPPORTED:fprintf(stderr, "read not supported\n");break;
		case RCP_ERROR_INVALID_AUTH_LEVEL:fprintf(stderr, "invalid authentication level\n");break;
		case RCP_ERROR_INVAILD_SESSION_ID:fprintf(stderr, "invalid session id\n");break;
		case RCP_ERROR_TRY_LATER:fprintf(stderr, "try later\n");break;
		case RCP_ERROR_COMMAND_SPECIFIC:fprintf(stderr, "command specific error\n");break;
		case RCP_ERROR_UNKNOWN:
		default:
			fprintf(stderr, "unknown error\n");break;
	}
}

int rcp_recv(rcp_packet* hdr)
{
	int res;
	int len;
	int received;

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
		//fprintf(stderr, "%d bytes received\n", res);
		received += res;
	}

	read_rcp_header(buffer, hdr);

	memcpy(hdr->payload, buffer+RCP_HEADER_LENGTH, hdr->payload_length);

	if (hdr->action == RCP_PACKET_ACTION_ERROR)
	{
		error_message(hdr->payload[0]);
		return -1;
	}

	return len;

error:
	fprintf(stderr, "ERROR : rcp_recv: %d - %s\n", errno, strerror(errno));
	return -1;
}
