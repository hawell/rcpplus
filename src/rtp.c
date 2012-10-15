/*
 * rtp.c
 *
 *  Created on: Aug 28, 2012
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

#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <limits.h>
#include <arpa/inet.h>

#include "rtp.h"
#include "rcplog.h"

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

#define NAL_START_FRAME_LENGTH	3
static unsigned char NAL_START_FRAME[] = {0x00, 0x00, 0x01};

static unsigned char sbit_mask[] = {0xFF, 0x7F, 0x3F, 0x1F, 0x0F, 0x07, 0x03, 0x01, 0x00};
static unsigned char ebit_mask[] = {0xFF, 0xFE, 0xFC, 0xF8, 0xF0, 0xE0, 0xC0, 0x80, 0x00};

int seq_cmp(int x, int y, rtp_merge_desc* mdesc)
{
	if (x == -INT_MAX)
		return -1;

	int t1 = ntohl(((rtp_header*)mdesc->fragment[x])->timestamp);
	int t2 = ntohl(((rtp_header*)mdesc->fragment[y])->timestamp);

	int s1 = ntohs(((rtp_header*)mdesc->fragment[x])->seq);
	int s2 = ntohs(((rtp_header*)mdesc->fragment[y])->seq);

	if (s1 < 100 && s2 > 65500)
		return 1;
	if (s2 < 100 && s1 > 65500)
		return -1;

	if (t1 > t2)
		return 1;
	else if (t1 < t2)
		return -1;
	else
	{
		if (s1 > s2)
			return 1;
		else if (s1 < s2)
			return -1;
	}

	return 0;
}

static void heap_init(rtp_merge_desc* mdesc)
{
	mdesc->heap_size = 0;
	mdesc->fragments_heap[0] = -INT_MAX;
}

static int heap_push(rtp_merge_desc* mdesc, int x)
{
	if (mdesc->heap_size == FRAGMENT_COUNT)
		return -1;

	mdesc->heap_size++;
	mdesc->fragments_heap[mdesc->heap_size] = x;
	int now = mdesc->heap_size;
	while (seq_cmp(mdesc->fragments_heap[now/2], x, mdesc) > 0)
	{
		mdesc->fragments_heap[now] = mdesc->fragments_heap[now/2];
		now /= 2;
	}
	mdesc->fragments_heap[now] = x;

	return 0;
}

static int heap_pop(rtp_merge_desc* mdesc)
{
	if (mdesc->heap_size == 0)
		return -1;

	int min, last, child, now;
	min = mdesc->fragments_heap[1];
	last = mdesc->fragments_heap[mdesc->heap_size--];

	for(now = 1; now*2 <= mdesc->heap_size; now = child)
	{
		child = now*2;
		if(child != mdesc->heap_size && seq_cmp(mdesc->fragments_heap[child+1], mdesc->fragments_heap[child], mdesc) < 0 )
				child++;

		if( seq_cmp(last, mdesc->fragments_heap[child], mdesc) > 0)
				mdesc->fragments_heap[now] = mdesc->fragments_heap[child];
		else
			break;
	}

	mdesc->fragments_heap[now] = last;

	return min;
}

static void queue_init(rtp_merge_desc* mdesc)
{
	for (int i=0; i<FRAGMENT_COUNT; i++)
		mdesc->fragments_queue[i] = i;
	mdesc->queue_start = 0;
	mdesc->queue_end = FRAGMENT_COUNT-1;
	mdesc->queue_size = FRAGMENT_COUNT;
}

static int queue_push(rtp_merge_desc* mdesc, int x)
{
	if (mdesc->queue_size == FRAGMENT_COUNT)
		return -1;

	mdesc->fragments_queue[mdesc->queue_end] = x;
	mdesc->queue_size++;
	mdesc->queue_end = (mdesc->queue_end+1) % FRAGMENT_COUNT;

	return 0;
}

static int queue_pop(rtp_merge_desc* mdesc)
{
	if (mdesc->queue_size == 0)
		return -1;

	int ret = mdesc->queue_start;
	mdesc->queue_start = (mdesc->queue_start+1) % FRAGMENT_COUNT;
	mdesc->queue_size--;

	return ret;
}

static int append_fu_a(unsigned char* data, int len, rtp_merge_desc* mdesc)
{
	fu_a_packet *fp = (fu_a_packet*)data;
	if (fp->fuh.s == 1)
	{
		DEBUG("start");
		if (mdesc->len == 0)
		{
			memcpy(mdesc->data+mdesc->len, NAL_START_FRAME, 3);
			mdesc->len += 3;

			nal_unit_header nh;
			nh.f = fp->nh.f;
			nh.nri = fp->nh.nri;
			nh.type = fp->fuh.type;

			DEBUG("nal unit: f=%d nri=%d type=%d", nh.f, nh.nri, nh.type);

			memcpy(mdesc->data+mdesc->len,&nh, 1);
			mdesc->len++;
			memcpy(mdesc->data + mdesc->len, fp->payload, len);
			mdesc->len += len;
			return 0;
		}
		else
		{
			ERROR("fragment unit is not the first unit");
			return -1;
		}
	}
	else
	{
		if (mdesc->len == 0)
		{
			ERROR("missing first fragment unit");
			return -1;
		}
		else
		{
			memcpy(mdesc->data+mdesc->len, fp->payload, len);
			mdesc->len += len;
		}
	}

	if (fp->fuh.e == 1)
	{
		DEBUG("end");
		mdesc->frame_complete = 1;
		return 1;
	}

	return 0;
}

static int append_stap_a(unsigned char* data, rtp_merge_desc* mdesc)
{
	assert(mdesc->len == 0);
	memcpy(mdesc->data + mdesc->len, NAL_START_FRAME, 3);
	mdesc->len += 3;
	data++;
	int size = ntohs(*(unsigned short*)(data));
	mdesc->data[mdesc->len] = *(data+2);
	mdesc->len++;
	memcpy(mdesc->data + mdesc->len, data+3, size);
	mdesc->len += size;

	memcpy(mdesc->data + mdesc->len, NAL_START_FRAME, 3);
	mdesc->len += 3;
	data += size + 2;
	size = ntohs(*(unsigned short*)(data));
	mdesc->data[mdesc->len] = *(data+2);
	mdesc->len++;
	memcpy(mdesc->data + mdesc->len, data+3, size);
	mdesc->len += size;

	mdesc->frame_complete = 1;

	return 0;
}

static int append_h263(unsigned char *fragment, int fragment_len, rtp_merge_desc* mdesc)
{
	static int ebit = 0;

	rtp_header *rtp_hdr = (rtp_header*)fragment;
	int rtp_header_len = RTP_HEADER_LENGTH_MANDATORY + rtp_hdr->cc*4;
	DEBUG("m=%d pt=%d hlen=%d len=%d seq=%d ts=%u", rtp_hdr->m, rtp_hdr->pt, rtp_header_len, fragment_len, ntohs(rtp_hdr->seq), ntohl(rtp_hdr->timestamp));

	if (mdesc->frame_error)
	{
		WARNING("frame skipped");
		mdesc->len = 0;
		if (rtp_hdr->m)
		{
			mdesc->frame_error = 0;
			ebit = 0;
		}
		return 0;
	}

	h263_header *h263_hdr = (h263_header*)(fragment+rtp_header_len);
	DEBUG("sbit=%d ebit=%d", h263_hdr->sbit, h263_hdr->ebit);
	unsigned char* pos = fragment+rtp_header_len;
	int len = fragment_len - rtp_header_len;
	if (h263_hdr->f == 0)	// type A
	{
		pos += 4;
		len -= 4;
	}
	else if (h263_hdr->p == 0)	// type B
	{
		pos += 8;
		len -= 8;
	}
	else	// type C
	{
		pos += 12;
		len -= 12;
	}
	//fwrite(pos, len, 1, stdout);

	DEBUG("%d %d", ebit, h263_hdr->sbit);
	//assert((mdesc->ebit + h263_hdr->sbit) % 8 == 0);
	if (((ebit + h263_hdr->sbit) % 8 != 0))
	{
		mdesc->frame_error = 1;
		ERROR("malformed frame received sbit=%d ebit=%d", h263_hdr->sbit, ebit);
		return 0;
	}
	if (h263_hdr->sbit)
	{
		mdesc->data[mdesc->len - 1] &= ebit_mask[ebit];
		pos[0] &= sbit_mask[h263_hdr->sbit];
		mdesc->data[mdesc->len - 1] |= pos[0];
		pos++;
		len--;
	}
	memcpy(mdesc->data+mdesc->len, pos, len);
	mdesc->len += len;
	ebit = h263_hdr->ebit;

	if (rtp_hdr->m == 1) // end of frame
	{
		mdesc->frame_complete = 1;
		mdesc->timestamp = ntohl(rtp_hdr->timestamp);
	}

	return 0;
}

static int append_h264(unsigned char *fragment, int fragment_len, rtp_merge_desc* mdesc)
{
	rtp_header *rtp = (rtp_header*)fragment;
	int rtp_header_len = RTP_HEADER_LENGTH_MANDATORY + rtp->cc*4;

	//DEBUG("v  - %d\np  - %d\nx  - %d\ncc - %d\nm  - %d\npt - %d\nsq - %d\n", rtp->version, rtp->p, rtp->x, rtp->cc, rtp->m, rtp->pt, rtp->seq);
	unsigned char nal_unit_header = *((unsigned char*)fragment + rtp_header_len);
	DEBUG("nal = %hhx", nal_unit_header);

	unsigned char nal_type = nal_unit_header & 0x1F;
	switch (nal_type)
	{
		case 24: // STAP-A
		{
			DEBUG("STAP-A");
			log_hex(RCP_LOG_DEBUG, "STAP-A", fragment + rtp_header_len, fragment_len-rtp_header_len);
			return append_stap_a(fragment+rtp_header_len, mdesc);
			//DEBUG("sps: %x %d", *(unsigned char*)&np[0].nh, np[0].size);
			//log_hex(LOG_DEBUG, "payload", np[0].payload, np[0].size);
			//DEBUG("pps: %x %d", *(unsigned char*)&np[1].nh, np[1].size);
			//log_hex(LOG_DEBUG, "payload", np[1].payload, np[1].size);
		}
		break;

		case 28: // FU-A
		{
			DEBUG("FU-A");
			return append_fu_a(fragment+rtp_header_len, fragment_len - (rtp_header_len+FU_A_HEADER_LENGTH), mdesc);
		}
		break;

		default:
			ERROR("unsupported nal type: %d", nal_type);
			break;
	}

	return 0;
}

int rtp_init(int type, rtp_merge_desc* mdesc)
{
	mdesc->len = 0;
	mdesc->type = type;
	mdesc->frame_complete = 0;
	mdesc->frame_error = 0;
	switch (type)
	{
		case RTP_PAYLOAD_TYPE_H263:
			mdesc->append = append_h263;
		break;

		case RTP_PAYLOAD_TYPE_H264:
			mdesc->append = append_h264;
		break;

		default:
			ERROR("unknown payload format");
			return -1;
		break;
	}

	queue_init(mdesc);
	heap_init(mdesc);

	return 0;
}

int rtp_recv(int socket, rtp_merge_desc* mdesc)
{
	int qp = queue_pop(mdesc);
	if (qp == -1)
		ERROR("cannot pop from queue");

	mdesc->fragment_size[qp] = recvfrom(socket, mdesc->fragment[qp], MTU_LENGTH, 0, NULL, NULL);

	int res = heap_push(mdesc, qp);
	if (res == -1)
		ERROR("cannot push to heap");

	int hp = heap_pop(mdesc);
	if (hp == -1)
		ERROR("cannot pop from heap");

	res = queue_push(mdesc, hp);
	if (res == -1)
		ERROR("cannot push to queue");

	if (mdesc->frame_complete)
	{
		WARNING("overwriting existing frame");
		mdesc->len = 0;
		mdesc->frame_complete = 0;
	}

	return mdesc->append(mdesc->fragment[hp], mdesc->fragment_size[hp], mdesc);
}

int rtp_pop_frame(video_frame* frame, rtp_merge_desc* mdesc)
{
	if (mdesc->frame_complete)
	{
		frame->data = mdesc->data;
		frame->len = mdesc->len;
		frame->timestamp = mdesc->timestamp;
		mdesc->frame_complete = 0;
		mdesc->len = 0;
		//mdesc->ebit = 0;

		return 0;
	}
	return -1;
}
