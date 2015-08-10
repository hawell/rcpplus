/*
 * device.c
 *
 *  Created on: Sep 9, 2012
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
#include <sys/time.h>
#include <unistd.h>
#include <tlog/tlog.h>

#include "device.h"
#include "rcpdefs.h"
#include "rcpplus.h"
#include "rcpcommand.h"

#define BC_AUTODETECT_PORT	1757
#define BC_ADDRESSCHANGE_PORT 1759
#define BC_COMMAND_PORT 1760

#define AUTODETECT_TIMEOUT	2200	// ms

static int broadcast(void* message, int len, unsigned short port)
{
	int res;

	int fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd == -1)
		goto error_socket;

	int broadcastEnable=1;
	res = setsockopt(fd, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable));
	if (res == -1)
		goto error_setopt;

	struct sockaddr_in addr;
	memset(&addr,0,sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
	addr.sin_port=htons(port);

	res = sendto(fd, message, len, 0, (const struct sockaddr*)&addr, sizeof(struct sockaddr_in));
	if (res == -1)
		goto error_send;

	close(fd);
	return 0;

error_setopt:
error_send:
	close(fd);

error_socket:
	TL_ERROR("broadcast() : %d - %s", errno, strerror(errno));
	return -1;
}

static char request_template[] = {
		0x99, 0x39, 0xA4, 0x27,
		0x00, 0x00, 0x00, 0x00, // sequence number
		0x00, 0x00,
		0x00, 0x00 // reply port
};

static char resp1_header_magic[] = {
		0x99, 0x39, 0xa4, 0x27
};

int autodetect(rcp_device** devs, int* num)
{
	int res;

	int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(0);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	res = bind(fd, (struct sockaddr*)&addr, sizeof(addr));
	if (res == -1)
	{
		TL_ERROR("cannot bind %d - %s\n", errno, strerror(errno));
		return -1;
	}

	listen(fd, 10);

	socklen_t len = sizeof(addr);
	getsockname(fd, (struct sockaddr*)&addr, &len);

	unsigned short reply_port = addr.sin_port;

	unsigned int seq_num = rand();

	memcpy(request_template+4, &seq_num, 4);
	memcpy(request_template+10, &reply_port, 2);

	broadcast(request_template, 12, BC_AUTODETECT_PORT);

	struct sockaddr_in si_remote;
	socklen_t slen = sizeof(si_remote);
	unsigned char buffer[1500];

	fd_set rfds;
	struct timeval tv;

	rcp_device* dev_list = NULL;
	int dev_count = 0;
	while (1)
	{
		FD_ZERO(&rfds);
		FD_SET(fd, &rfds);

		tv.tv_sec = AUTODETECT_TIMEOUT / 1000;
		tv.tv_usec = (AUTODETECT_TIMEOUT % 1000) * 1000;

		res = select(fd+1, &rfds, NULL, NULL, &tv);
		if (res == -1)
		{
			TL_ERROR("select : %d - %s", errno, strerror(errno));
			break;
		}
		else if (res == 0)
			break;

		int size = recvfrom(fd, buffer, 1500, 0, (struct sockaddr*)&si_remote, &slen);

		// this should never happen
		if (size < 4)
			continue;

		// ignore 2nd response packet
		if (memcmp(buffer, resp1_header_magic, 4) == 0)
		{
			tlog_hex(TLOG_DEBUG, "device", buffer, size);

			dev_list = (rcp_device*)realloc(dev_list, sizeof(rcp_device) * (dev_count+1));
			rcp_device* dev = &dev_list[dev_count++];

			memcpy(dev->hw_addr, buffer+8, 6);
			memcpy(dev->address, buffer+16, 4);
			memcpy(dev->netmask, buffer+20, 4);
			memcpy(dev->gateway, buffer+24, 4);

			dev->old_id = (unsigned char)buffer[15] >> 4;
			TL_INFO("%d", dev->old_id);
			dev->new_id = buffer[31];
			TL_INFO("%d", dev->new_id);
			dev->type = buffer[28];
			dev->connections = buffer[29];
		}
	}

	*devs = dev_list;
	*num = dev_count;

	return 0;
}

static char* nid_str(int id)
{
	switch (id)
	{
		case RCP_HARDWARE_ID_VIPX1:return "VIP X1";
		case RCP_HARDWARE_ID_VIPX2:return "VIP X2";
		case RCP_HARDWARE_ID_VIPXDEC:return "VIP XDEC";
		case RCP_HARDWARE_ID_VJ_X10:return "VJ X10";
		case RCP_HARDWARE_ID_VJ_X20:return "VJ X20";
		case RCP_HARDWARE_ID_VJ_X40:return "VJ X40";
		case RCP_HARDWARE_ID_VJ_X40_ECO:return "VJ X40 ECO";
		case RCP_HARDWARE_ID_VJ_X10_ECO:return "VJ X10 ECO";
		case RCP_HARDWARE_ID_VJ_X20_ECO:return "VJ X20 ECO";
		case RCP_HARDWARE_ID_IP_PANEL:return "IP Panel";
		case RCP_HARDWARE_ID_GEN4:return "Gen4";
		case RCP_HARDWARE_ID_M1600:return "M1600";
		case RCP_HARDWARE_ID_FLEXIDOME:return "FlexiDome";
		case RCP_HARDWARE_ID_M1600_DEC:return "M1600 DEC";
		case RCP_HARDWARE_ID_M1600_XFM:return "M1600 XFM";
		case RCP_HARDWARE_ID_AUTODOME:return "AutoDome";
		case RCP_HARDWARE_ID_NBC_225P:return "NBC 225P";
		case RCP_HARDWARE_ID_VIPX1_XF:return "VIPX1 XF";
		case RCP_HARDWARE_ID_NBC_225W:return "NBC 224W";
		case RCP_HARDWARE_ID_NBC_255P:return "NBC 255P";
		case RCP_HARDWARE_ID_NBC_255W:return "NBC 255W";
		case RCP_HARDWARE_ID_AUTODOME_EASY_II:return "AutoDome Easy II";
		case RCP_HARDWARE_ID_AUTODOME_EASY_II_E:return "AutoDome Easy II E";
		case RCP_HARDWARE_ID_VIPX1_XF_E:return "VIPX1 XF E";
		case RCP_HARDWARE_ID_VJT_X20XF_E_2CH:return "VIPX1 XFE 2ChH";
		case RCP_HARDWARE_ID_VJT_X20XF_E_4CH:return "VJT X20XF E 4CH";
		case RCP_HARDWARE_ID_VIPX1_XF_W:return "VIPX1 XF W";
		case RCP_HARDWARE_ID_VG5_AUTODOME_700:return "VG4 AutoDome 700";
		case RCP_HARDWARE_ID_NDC_455_P:return "NDC 455 P";
		case RCP_HARDWARE_ID_NDC_455_P_IVA:return "NDC 455 P IVA";
		case RCP_HARDWARE_ID_NBC_455_P:return "NBC 455 P";
		case RCP_HARDWARE_ID_NBC_455_P_IVA:return "NBC 455 P IVA";
		case RCP_HARDWARE_ID_VG4_AUTODOME:return "VG4 AutoDome";
		case RCP_HARDWARE_ID_NDC_225_P:return "NDC 225 P";
		case RCP_HARDWARE_ID_NDC_255_P_2:return "NDC 255 P";
		case RCP_HARDWARE_ID_VOT:return "VOT";
		case RCP_HARDWARE_ID_NDC_274_P:return "NDC 274 P";
		case RCP_HARDWARE_ID_NDC_284_P:return "NDC 284 P";
		case RCP_HARDWARE_ID_NTC_265_PI:return "NTC 265 PI";
		case RCP_HARDWARE_ID_NDC_265_PIO:return "NDC 265 PIO";
		case RCP_HARDWARE_ID_NDN_822:return "NDN 822";
		case RCP_HARDWARE_ID_DINION_720P:return "Dinion 720P";
		case RCP_HARDWARE_ID_FLEXIDOME_720P:return "FlexiDome 720P";
		case RCP_HARDWARE_ID_NDC_265_W:return "NDC 265 W";
		case RCP_HARDWARE_ID_NDC_265_P:return "NDC 265 P";
		case RCP_HARDWARE_ID_NBC_265_P:return "NDC 265 P";
		case RCP_HARDWARE_ID_NDC_225_PI:return "NDC 225 PI";
		case RCP_HARDWARE_ID_NTC_255_PI:return "NTC 255 PI";
		case RCP_HARDWARE_ID_JR_DOME_HD:return "JR Dome HD";
		case RCP_HARDWARE_ID_JR_DOME_HD_FIXED:return "JR Dome HD Fixed";
		case RCP_HARDWARE_ID_EX30_IR:return "EX30 IR";
		case RCP_HARDWARE_ID_GEN5_HD_PC:return "Gen5 HD PC";
		case RCP_HARDWARE_ID_EX65:return "EX65";
		case RCP_HARDWARE_ID_DINION_1080P:return "Dinion 1080P";
		case RCP_HARDWARE_ID_FLEXIDOME_1080P:return "FlexiDome 1080P";
		case RCP_HARDWARE_ID_HD_DECODER:return "HD Decoder";
		case RCP_HARDWARE_ID_GEN5_HD:return "Gen5 HD";
		case RCP_HARDWARE_ID_NER_L2:return "NER L2";
		case RCP_HARDWARE_ID_VIP_MIC:return "VIP MIC";
		case RCP_HARDWARE_ID_NBN_932:return "NBN 932";
		case RCP_HARDWARE_ID_TESLA_DOME:return "Tesla Dome";
		case RCP_HARDWARE_ID_GEN5_A5_700:return "Gen5 A5 700";
		case RCP_HARDWARE_ID_VJ_GENERIC_TRANSCODER:return "VJ Genric Transcoder";
		case RCP_HARDWARE_ID_HD_DECODER_M:return "HD Decoder M";
		case RCP_HARDWARE_ID_OASIS:return "Oasis";
		case RCP_HARDWARE_ID_GALILEO_BOXED:return "Galileo Boxed";
		case RCP_HARDWARE_ID_GALILEO_DOME:return "Galileo Dome";
		case RCP_HARDWARE_ID_HUYGENS_KEPPLER:return "HuyGens Keppler";
		case RCP_HARDWARE_ID_TESLA_KEPPLER:return "Tesla Keppler";
		case RCP_HARDWARE_ID_GALILEO_KEPPLER:return "Galileo Keppler";
		case RCP_HARDWARE_ID_NUC_20002:return "NUC 20002";
		case RCP_HARDWARE_ID_NUC_20012:return "NUC 20012";
		case RCP_HARDWARE_ID_NUC_50022:return "NUC 50022";
		case RCP_HARDWARE_ID_NUC_50051:return "NUC 50051";
		case RCP_HARDWARE_ID_NPC_20002:return "NPC 20002";
		case RCP_HARDWARE_ID_NPC_20012:return "NPC 20012";
		case RCP_HARDWARE_ID_NPC_20012_W:return "NPC 20012 W";
		case RCP_HARDWARE_ID_NIN_50022:return "NIN 50022";
		case RCP_HARDWARE_ID_NII_50022:return "NII 50022";
		case RCP_HARDWARE_ID_NDN_50022:return "NDN 50022";
		case RCP_HARDWARE_ID_NDI_50022:return "NDI 50022";
		case RCP_HARDWARE_ID_NTI_50022:return "NTI 50022";
		case RCP_HARDWARE_ID_NIN_50051:return "NIN 50051";
		case RCP_HARDWARE_ID_NDN_50051:return "NDN 50051";
		case RCP_HARDWARE_ID_NAI_90022:return "NAI 90022";
		case RCP_HARDWARE_ID_NCN_90022:return "NCN 90022";
		case RCP_HARDWARE_ID_NEVADA_DECODER:return "Nevada Decoder";
		case RCP_HARDWARE_ID_NBN_50022_C:return "NBN 50022 C";
		case RCP_HARDWARE_ID_NBN_40012_C:return "NBN 40012 C";
		case RCP_HARDWARE_ID_NBN_80052:return "NBN 80052";
		case RCP_HARDWARE_ID_MIC_7000:return "MIC 7000";
		case RCP_HARDWARE_ID_EX65_HD:return "EX65 HD";
		case RCP_HARDWARE_ID_NBN_80122:return "NBN 80122";
		case RCP_HARDWARE_ID_NPC_20012_L:return "NPC 20012 L";
		case RCP_HARDWARE_ID_MIC_NPS:return "MIC NPS";
		case RCP_HARDWARE_ID_NBN_50051_C:return "NBN 50051 C";
		case RCP_HARDWARE_ID_NIN_40012:return "NIN 40012";
		case RCP_HARDWARE_ID_NII_40012:return "NII 40012";
		case RCP_HARDWARE_ID_NDN_40012:return "NDN 40012";
		case RCP_HARDWARE_ID_NDI_40012:return "NDI 40012";
		case RCP_HARDWARE_ID_NTI_40012:return "NTI 40012";
		case RCP_HARDWARE_ID_NII_50051:return "NII 50051";
		case RCP_HARDWARE_ID_NDI_50051:return "NDI 50051";
		case RCP_HARDWARE_ID_NEZ_4000:return "NEZ 4000";
		case RCP_HARDWARE_ID_VEGA_3000_HD:return "Vega 3000 HD";
		case RCP_HARDWARE_ID_VEGA_4000_HD:return "Vega 4000 HD";
		case RCP_HARDWARE_ID_VEGA_5000_HD:return "Vega 5000 HD";
		case RCP_HARDWARE_ID_VEGA_5000_MP:return "Vega 5000 MP";
		case RCP_HARDWARE_ID_NIN_70122_180:return "NIN 70122 180";
		case RCP_HARDWARE_ID_NIN_70122_360:return "NIN 70122 360";
		case RCP_HARDWARE_ID_NEZ_5000:return "NEZ 5000";
		case RCP_HARDWARE_ID_NEZ_5000_IR:return "NEZ 5000 IR";
		case RCP_HARDWARE_ID_ROLA:return "Rola";
		case RCP_HARDWARE_ID_NBN_80122_CA:return "NBN 80122 CA";
		case RCP_HARDWARE_ID_ISCSI_TARGET:return "ISCSI Target";
		case RCP_HARDWARE_ID_VRM_PROXY_16:return "VRM Proxy 16";
		case RCP_HARDWARE_ID_VRM_UL_APP:return "VRM UL APP";
		case RCP_HARDWARE_ID_VRM_LE_APP:return "VRM LE APP";
		case RCP_HARDWARE_ID_CAMNETWORK:return "CamNetwork";
		case RCP_HARDWARE_ID_VRM_PROXY:return "VRM Proxy";
		case RCP_HARDWARE_ID_VRM:return "VRM";
		case RCP_HARDWARE_ID_VIDOS_SERVER:return "Vidos Server";
		case RCP_HARDWARE_ID_VIDOS_MONITOR:return "Vidos Monitor";
	}
	return "Unknown Device";
}

static char* oid_str(int id)
{
	switch (id)
	{
		case RCP_DEVICE_NCID_NVR:return "NCR";
		case RCP_DEVICE_NCID_VJ8000: return "VJ8000";
		case RCP_DEVICE_NCID_VIP10: return "VIP10";
		case RCP_DEVICE_NCID_VIP1000: return "VIP1000";
		case RCP_DEVICE_NCID_VJ400: return "VJ400";
		case RCP_DEVICE_NCID_VIP100: return "VIP100";
		case RCP_DEVICE_NCID_VJEX: return "VJEX";
		case RCP_DEVICE_NCID_VJ1000: return "VJ1000";
		case RCP_DEVICE_NCID_VJ100: return "VJ100";
		case RCP_DEVICE_NCID_VJ10: return "VJ10";
		case RCP_DEVICE_NCID_VJ8008: return "VJ8008";
		case RCP_DEVICE_NCID_VJ8004: return "VJ8004";
		case RCP_DEVICE_ESCAPECODE: return "New Id";
	}
	return "Unknown Device";
}

int get_device(rcp_device* dev)
{
	rcp_packet req;
	rcp_packet* resp;

	init_rcp_header(&req, 0, RCP_COMMAND_CONF_MAC_ADDRESS, RCP_COMMAND_MODE_READ, RCP_DATA_TYPE_P_OCTET);
	req.numeric_descriptor = 0;

	resp = rcp_command(&req);
	if (resp == NULL)
		goto error;

	memcpy(dev->hw_addr, resp->payload, resp->payload_length);

	init_rcp_header(&req, 0, RCP_COMMAND_CONF_IP_STR, RCP_COMMAND_MODE_READ, RCP_DATA_TYPE_P_STRING);
	req.numeric_descriptor = 0;

	resp = rcp_command(&req);
	if (resp == NULL)
		goto error;

	sscanf(resp->payload, "%hhu.%hhu.%hhu.%hhu", &dev->address[0], &dev->address[1], &dev->address[2], &dev->address[3]);

	init_rcp_header(&req, 0, RCP_COMMAND_CONF_GATEWAY_IP_STR, RCP_COMMAND_MODE_READ, RCP_DATA_TYPE_P_STRING);
	req.numeric_descriptor = 0;

	resp = rcp_command(&req);
	if (resp == NULL)
		goto error;

	sscanf(resp->payload, "%hhu.%hhu.%hhu.%hhu", &dev->gateway[0], &dev->gateway[1], &dev->gateway[2], &dev->gateway[3]);

	init_rcp_header(&req, 0, RCP_COMMAND_CONF_SUBNET, RCP_COMMAND_MODE_READ, RCP_DATA_TYPE_T_DWORD);
	req.numeric_descriptor = 0;

	resp = rcp_command(&req);
	if (resp == NULL)
		goto error;

	unsigned int tmp = ntohl(*resp->payload);
	memcpy(dev->netmask, &tmp, 4);

	init_rcp_header(&req, 0, RCP_COMMAND_CONF_HARDWARE_VERSION, RCP_COMMAND_MODE_READ, RCP_DATA_TYPE_P_STRING);
	req.numeric_descriptor = 0;

	resp = rcp_command(&req);
	if (resp == NULL)
		goto error;

	if (resp->payload[0] == 'F')
	{
		dev->old_id = RCP_DEVICE_ESCAPECODE;
		dev->new_id = resp->payload[4]>='A'?(resp->payload[4]-'A'+10)*16:(resp->payload[4]-'0')*16;
		dev->new_id += resp->payload[5]>='A'?(resp->payload[5]-'A'+10):(resp->payload[5]-'0');
	}
	else
	{
		dev->old_id = resp->payload[0]>='A'?(resp->payload[0]-'A'+10):(resp->payload[0]-'0');
		dev->new_id = 0;
	}

	dev->connections = 0;

	return 0;

error:
	TL_ERROR("get_device()");
	return -1;
}

int set_device_address(rcp_device* dev)
{
	unsigned char message[24];
	memset(message, 0, 24);

	memcpy(message, dev->hw_addr, 6);
	memcpy(message+8, dev->address, 4);
	memcpy(message+12, dev->netmask, 4);
	memcpy(message+16, dev->gateway, 4);

	return broadcast(message, 24, BC_ADDRESSCHANGE_PORT);
}

int broadcast_command(rcp_device* dev, int cmd, int data)
{
	unsigned char* message[16];
	memset(message, 0, 16);

	memcpy(message, dev->hw_addr, 6);

	unsigned short tmp16 = htons(cmd);
	memcpy(message+8, &tmp16, 2);

	tmp16 = htons(4);
	memcpy(message+10, &tmp16, 2);

	unsigned int tmp32 = htonl(data);
	memcpy(message+12, &tmp32, 4);

	return broadcast(message, 16, BC_COMMAND_PORT);
}

const char* get_device_type_str(rcp_device* dev)
{
	if (dev->old_id == RCP_DEVICE_ESCAPECODE)
		return nid_str(dev->new_id);
	else
		return oid_str(dev->old_id);
}

void log_device(int level, rcp_device* dev)
{
	char tmp[100];
	switch (dev->type)
	{
		case RCP_HARDWARE_TYPE_VIN:strcpy(tmp, "Video In");break;
		case RCP_HARDWARE_TYPE_VOUT:strcpy(tmp, "Video Out");break;
		case RCP_HARDWARE_TYPE_VIN_OUT:strcpy(tmp, "Video In/Out");break;
		default:strcpy(tmp, "Unknown Device Type");break;
	}
	tlog(level, "%-20s %s", "Device Type", tmp);
	tlog(level, "%-20s %s %d %d", "Device Name", get_device_type_str(dev), dev->old_id, dev->new_id);
	sprintf(tmp, "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx", dev->hw_addr[0], dev->hw_addr[1], dev->hw_addr[2], dev->hw_addr[3], dev->hw_addr[4], dev->hw_addr[5]);
	tlog(level, "%-20s %s", "MAC Address", tmp);
	sprintf(tmp, "%d.%d.%d.%d", dev->address[0], dev->address[1], dev->address[2], dev->address[3]);
	tlog(level, "%-20s %s", "Address", tmp);
	sprintf(tmp, "%d.%d.%d.%d", dev->netmask[0], dev->netmask[1], dev->netmask[2], dev->netmask[3]);
	tlog(level, "%-20s %s", "Subnet Mask", tmp);
	sprintf(tmp, "%d.%d.%d.%d", dev->gateway[0], dev->gateway[1], dev->gateway[2], dev->gateway[3]);
	tlog(level, "%-20s %s", "Gateway", tmp);
	tlog(level, "%-20s %d", "Connections", dev->connections);
}
