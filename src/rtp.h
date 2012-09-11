/*
 * rtp.h
 *
 *  Created on: Aug 27, 2012
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

/*
 * WARNING: this is a minimal implementation of rtp payload format defined in rfc 3984 for h264 and sould not be 
 * considered as part of rcpplus.
 * a practical application should use a full implenetation, this file is only for test purposes.
*/

#ifndef RTP_H_
#define RTP_H_

#define NAL_START_FRAME_LENGTH	3
extern unsigned char NAL_START_FRAME[];

#define RTP_HEADER_LENGTH_MANDATORY		12

typedef struct {
    unsigned char cc:4;         // CSRC count
    unsigned char x:1;          // header extension flag
    unsigned char p:1;          // padding flag
    unsigned char version:2;    // protocol version
    unsigned char pt:7;         // payload type
    unsigned char m:1;          // marker bit
    unsigned short seq;         // sequence number, network order
    unsigned int timestamp;     // timestamp, network order
    unsigned int ssrc;          // synchronization source, network order
    unsigned int csrc[];        // optional CSRC list
} rtp_header;

/****** h264 payload ******/

typedef struct {
	unsigned char type:5;
	unsigned char nri:2;
	unsigned char f:1;
} nal_unit_header;

#define MAX_NAL_FRAME_LENGTH	200000

typedef struct {
	nal_unit_header nh;
	int size;
	unsigned char payload[MAX_NAL_FRAME_LENGTH];
} nal_packet;

typedef struct {
	unsigned char type:5;
	unsigned char r:1;
	unsigned char e:1;
	unsigned char s:1;
} fu_header;

#define FU_A_HEADER_LENGTH	2

typedef struct {
	nal_unit_header nh;
	fu_header fuh;
	unsigned char payload[MAX_NAL_FRAME_LENGTH];
} fu_a_packet;

int defrag(nal_packet* np, void* buffer, int len);

/**************************/

typedef struct {
	unsigned char ebit:3;
	unsigned char sbit:3;
	unsigned char p:1;
	unsigned char f:1;
} h263_header;

typedef struct {
	unsigned int tr:8;
	unsigned int trb:3;
	unsigned int dbq:2;
	unsigned int r:4;
	unsigned int a:1;
	unsigned int s:1;
	unsigned int u:1;
	unsigned int i:1;
	unsigned int src:3;
	unsigned int ebit:3;
	unsigned int sbit:3;
	unsigned int p:1;
	unsigned int f:1;
} h263_a;

typedef struct {
	unsigned char ebit:3;
	unsigned char sbit:3;
	unsigned char p:1;
	unsigned char f:1;

} h263_b;
#endif /* RTP_H_ */
