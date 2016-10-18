/*
 * rcpdefs.h
 *
 *  Created on: Aug 15, 2012
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

#ifndef _RCP_DEFS_H_
#define _RCP_DEFS_H_

#define UNUSED(x)	(void)(x)

#define RCP_VERSION					3

#define RCP_CONTROL_PORT			1756

#define TPKT_VERSION				3
#define TPKT_HEADER_LENGTH			4

#define RCP_HEADER_LENGTH			16

#define RCP_MAX_PACKET_LEN			5000

#define RCP_MD5_RANDOM_LEN			16

#define RCP_DATA_TYPE_F_FLAG		0x00 // (1 Byte)
#define RCP_DATA_TYPE_T_OCTET		0x01 // (1 Byte)
#define RCP_DATA_TYPE_T_WORD		0x02 // (2 Byte)
#define RCP_DATA_TYPE_T_INT			0x04 // (4 Byte)
#define RCP_DATA_TYPE_T_DWORD		0x08 // (4 Byte)
#define RCP_DATA_TYPE_P_OCTET		0x0C // (N Byte)
#define RCP_DATA_TYPE_P_STRING		0x10 // (N Byte)
#define RCP_DATA_TYPE_P_UNICODE		0x14 // (N Byte)

#define RCP_MEDIA_TYPE_VIDEO		0x01
#define RCP_MEDIA_TYPE_AUDIO		0x02
#define RCP_MEDIA_TYPE_DATA			0x03

#define RCP_CONNECTION_METHOD_GET			0x00
#define RCP_CONNECTION_METHOD_PUT			0x01
#define RCP_CONNECTION_METHOD_KEYTRANSPORT	0xE0

#define RCP_MEP_UDP					0x01
#define RCP_MEP_TCP					0x04

#define RCP_COMMAND_MODE_READ		0x00
#define RCP_COMMAND_MODE_WRITE		0x01

#define RCP_PACKET_ACTION_REQUEST	0x00
#define RCP_PACKET_ACTION_REPLY		0x01
#define RCP_PACKET_ACTION_MESSAGE	0x02
#define RCP_PACKET_ACTION_ERROR		0x03

#define RCP_USER_LEVEL_USER			0x01 // for video access and control over the video stream (e.g. caminput)
#define RCP_USER_LEVEL_SERVICE		0x02 // for video and all administrative access
#define RCP_USER_LEVEL_LIVE			0x03 // for video access only; no control

#define RCP_REGISTRATION_TYPE_NORMAL	0x01
#define RCP_REGISTRATION_TYPE_HOOKON	0x02
#define RCP_REGISTRATION_TYPE_HOOKBACK	0x03

#define RCP_ENCRYPTION_MODE_PLAIN		0x00
#define RCP_ENCRYPTION_MODE_MD5			0x01

#define RCP_CODER_DECODER				0x00
#define RCP_CODER_ENCODER				0x01

#define RCP_ERROR_UNKNOWN				0xFF
#define RCP_ERROR_INVALID_VERSION		0x10
#define RCP_ERROR_NOT_REGISTERED		0x20
#define RCP_ERROR_INVALID_CLIENT_ID		0x21
#define RCP_ERROR_INVALID_METHOD		0x30
#define RCP_ERROR_INVALID_CMD			0x40
#define RCP_ERROR_INVALID_ACCESS_TYPE	0x50
#define RCP_ERROR_INVALID_DATA_TYPE		0x60
#define RCP_ERROR_WRITE_ERROR			0x70
#define RCP_ERROR_PACKET_SIZE			0x80
#define RCP_ERROR_READ_NOT_SUPPORTED	0x90
#define RCP_ERROR_INVALID_AUTH_LEVEL	0xa0
#define RCP_ERROR_INVAILD_SESSION_ID	0xb0
#define RCP_ERROR_TRY_LATER				0xc0
#define RCP_ERROR_COMMAND_SPECIFIC		0xe0

#define RCP_VIDEO_INPUT_FORMAT_NONE     0
#define RCP_VIDEO_INPUT_FORMAT_PAL      1
#define RCP_VIDEO_INPUT_FORMAT_NTSC     2
#define RCP_VIDEO_INPUT_FORMAT_VGA      3
#define RCP_VIDEO_INPUT_FORMAT_720P     4
#define RCP_VIDEO_INPUT_FORMAT_1080P    5
#define RCP_VIDEO_INPUT_FORMAT_QVGA     6

#define RCP_VIDEO_RESOLUTION_QCIF			0x0001
#define RCP_VIDEO_RESOLUTION_CIF			0x0002
#define RCP_VIDEO_RESOLUTION_2CIF			0x0004
#define RCP_VIDEO_RESOLUTION_4CIF			0x0008
#define RCP_VIDEO_RESOLUTION_CUSTOM		0x0010
#define RCP_VIDEO_RESOLUTION_QVGA			0x0020
#define RCP_VIDEO_RESOLUTION_VGA			0x0040
#define RCP_VIDEO_RESOLUTION_HD720			0x0080
#define RCP_VIDEO_RESOLUTION_HD1080		0x0100
#define RCP_VIDEO_RESOLUTION_WD144			0x0200
#define RCP_VIDEO_RESOLUTION_WD288			0x0400
#define RCP_VIDEO_RESOLUTION_WD432			0x0800
#define RCP_VIDEO_RESOLUTION_HD2592x1944	0x1000

#define RCP_COMMAND_CONF_RCP_CLIENT_REGISTRATION		 0xff00
#define RCP_COMMAND_CONF_RCP_CLIENT_UNREGISTER			 0xff01
#define RCP_COMMAND_CONF_RCP_CLIENT_TIMEOUT_WARNING	 0xff03
#define RCP_COMMAND_CONF_RCP_REG_MD5_RANDOM			 0xff05
#define RCP_COMMAND_CONF_RCP_TRANSFER_TRANSPARENT_DATA	 0xffdd
#define RCP_COMMAND_CONF_CONNECT_PRIMITIVE				 0xff0c
#define RCP_COMMAND_CONF_DISCONNECT_PRIMITIVE			 0xff0d
#define RCP_COMMAND_CONF_CONNECT_TO					 0xffcc
#define RCP_COMMAND_CONF_ACTIVE_CONNECTION_LIST		 0xffc1
#define RCP_COMMAND_CONF_MEDIA_SOCKETS_COMPLETE		 0xffc7
#define RCP_COMMAND_CONF_RCP_CONNECTIONS_ALIVE			 0xffc2
#define RCP_COMMAND_CONF_CAPABILITY_LIST				 0xff10
#define RCP_COMMAND_CONF_MPEG4_INTRA_FRAME_REQUEST		 0x0605
#define RCP_COMMAND_CONF_ENC_GET_SPS_AND_PPS			 0x0a1d
#define RCP_COMMAND_CONF_PASSWORD_SETTINGS              0x028b
#define RCP_COMMAND_CONF_DEFAULTS                       0x093d
#define RCP_COMMAND_CONF_FACTORY_DEFAULTS               0x09a0
#define RCP_COMMAND_CONF_BOARD_RESET                    0x0811
#define RCP_COMMAND_CONF_VIDEO_INPUT_FORMAT             0x0504
#define RCP_COMMAND_CONF_VIDEO_INPUT_FORMAT_EX          0x0b10
#define RCP_COMMAND_CONF_VIDEO_OUT_STANDARD             0x0700
#define RCP_COMMAND_CONF_VIN_BASE_FRAMERATE             0x0ad6

#define RCP_COMMAND_CONF_JPEG					0x099e
#define RCP_COMMAND_CONF_MPEG4_CURRENT_PARAMS	0x0600


#endif /* _RCP_DEFS_H_ */
