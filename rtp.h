/*
 * rtp.h
 *
 *  Created on: Aug 27, 2012
 *      Author: arash
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

typedef struct {
	unsigned char type:5;
	unsigned char nri:2;
	unsigned char f:1;
} nal_unit_header;

#define MAX_NAL_FRAME_LENGTH	65536

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

#endif /* RTP_H_ */
