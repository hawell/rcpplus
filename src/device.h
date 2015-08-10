/*
 * device.h
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

#ifndef DEVICE_H_
#define DEVICE_H_

#define RCP_DEVICE_NCID_NVR				0x1
#define RCP_DEVICE_NCID_VJ8000			0x2
#define RCP_DEVICE_NCID_VIP10			0x3
#define RCP_DEVICE_NCID_VIP1000			0x4
#define RCP_DEVICE_NCID_VJ400			0x6
#define RCP_DEVICE_NCID_VIP100			0x7
#define RCP_DEVICE_NCID_VJEX			0x8
#define RCP_DEVICE_NCID_VJ1000			0x9
#define RCP_DEVICE_NCID_VJ100			0xA
#define RCP_DEVICE_NCID_VJ10			0xB
#define RCP_DEVICE_NCID_VJ8008			0xC
#define RCP_DEVICE_NCID_VJ8004			0xD // (latest unit carrying the old numbering scheme)
#define RCP_DEVICE_ESCAPECODE			0xF // (used to switch over to new detection)

#define RCP_HARDWARE_TYPE_VIN			0x0
#define RCP_HARDWARE_TYPE_VOUT			0x1
#define RCP_HARDWARE_TYPE_VIN_OUT		0x2

#define RCP_HARDWARE_ID_VIPX1						0x01
#define RCP_HARDWARE_ID_VIPX2						0x02
#define RCP_HARDWARE_ID_VIPXDEC					0x03
#define RCP_HARDWARE_ID_VJ_X10						0x05
#define RCP_HARDWARE_ID_VJ_X20						0x06
#define RCP_HARDWARE_ID_VJ_X40						0x07
#define RCP_HARDWARE_ID_VJ_X40_ECO					0x08
#define RCP_HARDWARE_ID_VJ_X10_ECO					0x09
#define RCP_HARDWARE_ID_VJ_X20_ECO					0x0a
#define RCP_HARDWARE_ID_IP_PANEL					0x0d
#define RCP_HARDWARE_ID_GEN4						0x0e
#define RCP_HARDWARE_ID_M1600						0x0f
#define RCP_HARDWARE_ID_FLEXIDOME					0x11
#define RCP_HARDWARE_ID_M1600_DEC					0x13
#define RCP_HARDWARE_ID_M1600_XFM					0x15
#define RCP_HARDWARE_ID_AUTODOME					0x16
#define RCP_HARDWARE_ID_NBC_225P					0x1a
#define RCP_HARDWARE_ID_VIPX1_XF					0x1e
#define RCP_HARDWARE_ID_NBC_225W					0x1f
#define RCP_HARDWARE_ID_NBC_255P					0x20
#define RCP_HARDWARE_ID_NBC_255W					0x21
#define RCP_HARDWARE_ID_AUTODOME_EASY_II			0x23
#define RCP_HARDWARE_ID_AUTODOME_EASY_II_E			0x24
#define RCP_HARDWARE_ID_VIPX1_XF_E					0x25
#define RCP_HARDWARE_ID_VJT_X20XF_E_2CH			0x26
#define RCP_HARDWARE_ID_VJT_X20XF_E_4CH			0x27
#define RCP_HARDWARE_ID_VIPX1_XF_W					0x28
#define RCP_HARDWARE_ID_VG5_AUTODOME_700			0x29
#define RCP_HARDWARE_ID_NDC_455_P					0x2a
#define RCP_HARDWARE_ID_NDC_455_P_IVA				0x2b
#define RCP_HARDWARE_ID_NBC_455_P					0x2c
#define RCP_HARDWARE_ID_NBC_455_P_IVA				0x2d
#define RCP_HARDWARE_ID_VG4_AUTODOME				0x2e
#define RCP_HARDWARE_ID_NDC_225_P					0x2f
#define RCP_HARDWARE_ID_NDC_255_P_2				0x30
#define RCP_HARDWARE_ID_VOT						0x32
#define RCP_HARDWARE_ID_NDC_274_P					0x35
#define RCP_HARDWARE_ID_NDC_284_P					0x36
#define RCP_HARDWARE_ID_NTC_265_PI					0x37
#define RCP_HARDWARE_ID_NDC_265_PIO				0x38
#define RCP_HARDWARE_ID_DINION_720P				0x39
#define RCP_HARDWARE_ID_NDN_822					0x3a
#define RCP_HARDWARE_ID_FLEXIDOME_720P				0x3b
#define RCP_HARDWARE_ID_NDC_265_W					0x3c
#define RCP_HARDWARE_ID_NDC_265_P					0x3d
#define RCP_HARDWARE_ID_NBC_265_P					0x3e
#define RCP_HARDWARE_ID_NDC_225_PI					0x3f
#define RCP_HARDWARE_ID_NTC_255_PI					0x40
#define RCP_HARDWARE_ID_JR_DOME_HD					0x41
#define RCP_HARDWARE_ID_JR_DOME_HD_FIXED			0x42
#define RCP_HARDWARE_ID_EX30_IR					0x43
#define RCP_HARDWARE_ID_GEN5_HD_PC					0x45
#define RCP_HARDWARE_ID_EX65						0x46
#define RCP_HARDWARE_ID_DINION_1080P				0x47
#define RCP_HARDWARE_ID_FLEXIDOME_1080P			0x48
#define RCP_HARDWARE_ID_HD_DECODER					0x49
#define RCP_HARDWARE_ID_GEN5_HD					0x4a
#define RCP_HARDWARE_ID_NER_L2						0x4b
#define RCP_HARDWARE_ID_VIP_MIC					0x4c
#define RCP_HARDWARE_ID_NBN_932					0x4f
#define RCP_HARDWARE_ID_TESLA_DOME					0x50
#define RCP_HARDWARE_ID_GEN5_A5_700				0x52
#define RCP_HARDWARE_ID_VJ_GENERIC_TRANSCODER		0x53
#define RCP_HARDWARE_ID_HD_DECODER_M				0x54
#define RCP_HARDWARE_ID_OASIS						0x55
#define RCP_HARDWARE_ID_GALILEO_BOXED				0x56
#define RCP_HARDWARE_ID_GALILEO_DOME				0x57
#define RCP_HARDWARE_ID_HUYGENS_KEPPLER			0x58
#define RCP_HARDWARE_ID_TESLA_KEPPLER				0x59
#define RCP_HARDWARE_ID_GALILEO_KEPPLER			0x5a
#define RCP_HARDWARE_ID_NUC_20002					0x5b
#define RCP_HARDWARE_ID_NUC_20012					0x5c
#define RCP_HARDWARE_ID_NUC_50022					0x5d
#define RCP_HARDWARE_ID_NUC_50051					0x5e
#define RCP_HARDWARE_ID_NPC_20002					0x5f
#define RCP_HARDWARE_ID_NPC_20012					0x60
#define RCP_HARDWARE_ID_NPC_20012_W				0x61
#define RCP_HARDWARE_ID_NIN_50022					0x62
#define RCP_HARDWARE_ID_NII_50022					0x63
#define RCP_HARDWARE_ID_NDN_50022					0x64
#define RCP_HARDWARE_ID_NDI_50022					0x65
#define RCP_HARDWARE_ID_NTI_50022					0x66
#define RCP_HARDWARE_ID_NIN_50051					0x67
#define RCP_HARDWARE_ID_NDN_50051					0x68
#define RCP_HARDWARE_ID_NAI_90022					0x69
#define RCP_HARDWARE_ID_NCN_90022					0x6a
#define RCP_HARDWARE_ID_NEVADA_DECODER				0x6c
#define RCP_HARDWARE_ID_NBN_50022_C				0x6d
#define RCP_HARDWARE_ID_NBN_40012_C				0x6e
#define RCP_HARDWARE_ID_NBN_80052					0x71
#define RCP_HARDWARE_ID_MIC_7000					0x72
#define RCP_HARDWARE_ID_EX65_HD					0x73
#define RCP_HARDWARE_ID_NBN_80122					0x74
#define RCP_HARDWARE_ID_NPC_20012_L				0x75
#define RCP_HARDWARE_ID_MIC_NPS					0x76
#define RCP_HARDWARE_ID_NBN_50051_C				0x78
#define RCP_HARDWARE_ID_NIN_40012					0x79
#define RCP_HARDWARE_ID_NII_40012					0x7a
#define RCP_HARDWARE_ID_NDN_40012					0x7b
#define RCP_HARDWARE_ID_NDI_40012					0x7c
#define RCP_HARDWARE_ID_NTI_40012					0x7d
#define RCP_HARDWARE_ID_NII_50051					0x7e
#define RCP_HARDWARE_ID_NDI_50051					0x7f
#define RCP_HARDWARE_ID_NEZ_4000					0x80
#define RCP_HARDWARE_ID_VEGA_3000_HD				0x81
#define RCP_HARDWARE_ID_VEGA_4000_HD				0x82
#define RCP_HARDWARE_ID_VEGA_5000_HD				0x83
#define RCP_HARDWARE_ID_VEGA_5000_MP				0x84
#define RCP_HARDWARE_ID_NIN_70122_180				0x85
#define RCP_HARDWARE_ID_NIN_70122_360				0x86
#define RCP_HARDWARE_ID_NEZ_5000					0x88
#define RCP_HARDWARE_ID_NEZ_5000_IR				0x89
#define RCP_HARDWARE_ID_ROLA						0x8b
#define RCP_HARDWARE_ID_NBN_80122_CA				0x8c

#define RCP_HARDWARE_ID_ISCSI_TARGET				0xf6
#define RCP_HARDWARE_ID_VRM_PROXY_16				0xf7
#define RCP_HARDWARE_ID_VRM_UL_APP					0xf8
#define RCP_HARDWARE_ID_VRM_LE_APP					0xf9
#define RCP_HARDWARE_ID_CAMNETWORK					0xfa
#define RCP_HARDWARE_ID_VRM_PROXY					0xfb
#define RCP_HARDWARE_ID_VRM						0xfc
#define RCP_HARDWARE_ID_VIDOS_SERVER				0xfd
#define RCP_HARDWARE_ID_VIDOS_MONITOR				0xfe


#define RCP_COMMAND_CONF_MAC_ADDRESS			0x00bc
#define RCP_COMMAND_CONF_IP_STR				0x007c
#define RCP_COMMAND_CONF_SUBNET				0x0002
#define RCP_COMMAND_CONF_GATEWAY_IP_STR		0x007f
#define RCP_COMMAND_CONF_HARDWARE_VERSION		0x002e

#define BC_COMMAND_LED_BLINK 0x0000 // Requests the unit blink the power led; duration is limited to 60 sec.
#define BC_COMMAND_SWITCH_DHCP 0x0001 // Switches DHCP on (payload = 1) or off (payload=0).

typedef struct {
	int old_id;
	int new_id;

	unsigned char hw_addr[6];

	unsigned char address[4];
	unsigned char netmask[4];
	unsigned char gateway[4];

	int type;
	int connections;
} rcp_device;

int autodetect(rcp_device** devs, int* num);

int get_device(rcp_device* dev);

const char* get_device_type_str(rcp_device* dev);

int set_device_address(rcp_device* dev);

int broadcast_command(rcp_device* dev, int cmd, int data);

void log_device(int level, rcp_device* dev);

#endif /* DEVICE_H_ */
