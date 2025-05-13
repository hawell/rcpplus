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
#include <tlog/tlog.h>
#include <errno.h>
#include <signal.h>

#include "rtp.h"

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

static unsigned char NAL_START_FRAME[] = {0x00, 0x00, 0x00, 0x01};

static unsigned char sbit_mask[] = {0xFF, 0x7F, 0x3F, 0x1F, 0x0F, 0x07, 0x03, 0x01, 0x00};
static unsigned char ebit_mask[] = {0xFF, 0xFE, 0xFC, 0xF8, 0xF0, 0xE0, 0xC0, 0x80, 0x00};

int seq_cmp(int x, int y, rtp_merge_desc* mdesc)
{
	if (x == -INT_MAX)
		return -1;

	const int t1 = ntohl(((rtp_header*)mdesc->fragment[x])->timestamp);
	const int t2 = ntohl(((rtp_header*)mdesc->fragment[y])->timestamp);

	const int s1 = ntohs(((rtp_header*)mdesc->fragment[x])->seq);
	const int s2 = ntohs(((rtp_header*)mdesc->fragment[y])->seq);

	if (s1 < 100 && s2 > 65500)
		return 1;
	if (s2 < 100 && s1 > 65500)
		return -1;

	if (t1 > t2)
		return 1;

	if (t1 < t2)
		return -1;

	if (s1 > s2)
		return 1;

	if (s1 < s2)
		return -1;

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

	int child, now;
	const int min = mdesc->fragments_heap[1];
	const int last = mdesc->fragments_heap[mdesc->heap_size--];

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
	mdesc->queue_start = (mdesc->queue_start + 1) % FRAGMENT_COUNT;
	mdesc->queue_size--;

	return ret;
}

static int append_fu_a(const char* data, int len, rtp_merge_desc* mdesc)
{
	const fu_a_packet *fp = (fu_a_packet*)data;
	if (fp->fuh.s == 1)
	{
		TL_DEBUG("start");
		if (mdesc->len != 0)
		{
			TL_WARNING("fragment unit is not the first unit");
			mdesc->len = 0;
		}

		if (mdesc->prepend_mpeg4_starter)
		{
			memcpy(mdesc->data + mdesc->len, NAL_START_FRAME, 4);
			mdesc->len += 4;
		}

		nal_unit_header nh;
		nh.f = fp->nh.f;
		nh.nri = fp->nh.nri;
		nh.type = fp->fuh.type;

		TL_DEBUG("nal unit: f=%d nri=%d type=%d", nh.f, nh.nri, nh.type);

		memcpy(mdesc->data + mdesc->len,&nh, 1);
		mdesc->len++;
		memcpy(mdesc->data + mdesc->len, fp->payload, len);
		mdesc->len += len;
		return 0;
	}
	else
	{
		if (mdesc->len == 0)
		{
			TL_ERROR("missing first fragment unit");
			return -1;
		}
		else
		{
			memcpy(mdesc->data + mdesc->len, fp->payload, len);
			mdesc->len += len;
		}
	}

	if (fp->fuh.e == 1)
	{
		TL_DEBUG("end");
		mdesc->frame_complete = 1;
		return 0;
	}

	return 0;
}

static int append_single_nal(const char* data, int data_len, rtp_merge_desc* mdesc)
{
	mdesc->len = 0;
	if (mdesc->prepend_mpeg4_starter)
	{
		memcpy(mdesc->data + mdesc->len, NAL_START_FRAME, 4);
		mdesc->len += 4;
	}
	memcpy(mdesc->data + mdesc->len, data, data_len);
	mdesc->len += data_len;

	mdesc->frame_complete = 1;

	return 0;
}

static int append_stap_a(char* data, rtp_merge_desc* mdesc)
{
	mdesc->len = 0;
	if (mdesc->prepend_mpeg4_starter)
	{
		memcpy(mdesc->data + mdesc->len, NAL_START_FRAME, 4);
		mdesc->len += 4;
	}
	data++;
	int size = ntohs(*(unsigned short*)(data));
	mdesc->data[mdesc->len] = *(data+2);
	mdesc->len++;
	memcpy(mdesc->data + mdesc->len, data+3, size);
	mdesc->len += size;

	//memcpy(mdesc->data + mdesc->len, NAL_START_FRAME, 3);
	//mdesc->len += 3;
	data += size + 2;
	size = ntohs(*(unsigned short*)(data));
	mdesc->data[mdesc->len] = *(data+2);
	mdesc->len++;
	memcpy(mdesc->data + mdesc->len, data+3, size);
	mdesc->len += size;

	mdesc->frame_complete = 1;

	return 0;
}

static int append_h263(char *fragment, int fragment_len, rtp_merge_desc* mdesc)
{
	rtp_header *rtp_hdr = (rtp_header*)fragment;
	int rtp_header_len = RTP_HEADER_LENGTH_MANDATORY + rtp_hdr->cc*4;
	TL_DEBUG("m=%d pt=%d hlen=%d len=%d seq=%d ts=%u", rtp_hdr->m, rtp_hdr->pt, rtp_header_len, fragment_len, ntohs(rtp_hdr->seq), ntohl(rtp_hdr->timestamp));

	if (mdesc->frame_error)
	{
		TL_WARNING("frame skipped");
		mdesc->len = 0;
		if (rtp_hdr->m)
		{
			mdesc->frame_error = 0;
			mdesc->ebit = 0;
		}
		return 0;
	}

	h263_header *h263_hdr = (h263_header*)(fragment+rtp_header_len);
	TL_DEBUG("sbit=%d ebit=%d", h263_hdr->sbit, h263_hdr->ebit);
	char* pos = fragment+rtp_header_len;
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

	TL_DEBUG("%d %d", mdesc->ebit, h263_hdr->sbit);
	//assert((mdesc->ebit + h263_hdr->sbit) % 8 == 0);
	if (((mdesc->ebit + h263_hdr->sbit) % 8 != 0))
	{
		mdesc->frame_error = 1;
		TL_ERROR("malformed frame received sbit=%d ebit=%d", h263_hdr->sbit, mdesc->ebit);
		return 0;
	}

	if (h263_hdr->sbit)
	{
		mdesc->data[mdesc->len - 1] &= ebit_mask[mdesc->ebit];
		pos[0] &= sbit_mask[h263_hdr->sbit];
		mdesc->data[mdesc->len - 1] |= pos[0];
		pos++;
		len--;
	}

	memcpy(mdesc->data+mdesc->len, pos, len);
	mdesc->len += len;
	mdesc->ebit = h263_hdr->ebit;

	if (rtp_hdr->m == 1) // end of frame
	{
		mdesc->frame_complete = 1;
		mdesc->timestamp = ntohl(rtp_hdr->timestamp);
	}

	return 0;
}

static int append_h264(char *fragment, int fragment_len, rtp_merge_desc* mdesc)
{
	rtp_header *rtp = (rtp_header*)fragment;
	const int rtp_header_len = RTP_HEADER_LENGTH_MANDATORY + rtp->cc*4;

	TL_DEBUG("v - %d p - %d x - %d cc - %d m  - %d pt - %d sq - %d",
		rtp->version, rtp->p, rtp->x, rtp->cc, rtp->m, rtp->pt, rtp->seq);

	unsigned char nal_unit_header = *((unsigned char*)fragment + rtp_header_len);
	TL_DEBUG("nal = %hhx", nal_unit_header);

	unsigned char nal_type = nal_unit_header & 0x1F;
	if (nal_type < 24)
	{
		// Single NAL Unit
		return append_single_nal(fragment + rtp_header_len,  fragment_len - rtp_header_len, mdesc);
	}
	else
	{
		switch (nal_type)
		{
			case 24: // STAP-A
			{
				TL_DEBUG("STAP-A");
				tlog_hex(TLOG_DEBUG, "STAP-A", fragment + rtp_header_len,
					fragment_len-rtp_header_len);
				return append_stap_a(fragment+rtp_header_len, mdesc);
				//TL_DEBUG("sps: %x %d", *(unsigned char*)&np[0].nh, np[0].size);
				//tlog_hex(LOG_DEBUG, "payload", np[0].payload, np[0].size);
				//TL_DEBUG("pps: %x %d", *(unsigned char*)&np[1].nh, np[1].size);
				//tlog_hex(LOG_DEBUG, "payload", np[1].payload, np[1].size);
			}
			break;

			case 28: // FU-A
			{
				TL_DEBUG("FU-A");
				return append_fu_a(fragment + rtp_header_len,
					fragment_len - (rtp_header_len + FU_A_HEADER_LENGTH), mdesc);
			}
			break;

			default:
				TL_ERROR("unsupported nal type: %d", nal_type);
				return -1;
				break;
		}
	}

	return 0;
}

int rtp_init(int type, int prepend_mpeg4_starter, rtp_merge_desc* mdesc)
{
	mdesc->ebit = 0;
	mdesc->len = 0;
	mdesc->type = type;
	mdesc->frame_complete = 0;
	mdesc->frame_error = 0;
	mdesc->prepend_mpeg4_starter = prepend_mpeg4_starter;
	switch (type)
	{
		case RTP_PAYLOAD_TYPE_H263:
			mdesc->append = append_h263;
		break;

		case RTP_PAYLOAD_TYPE_H264:
			mdesc->append = append_h264;
		break;

		default:
			TL_ERROR("unknown payload format");
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
	{
		TL_ERROR("cannot pop from queue");
		goto error;
	}

	mdesc->fragment_size[qp] = recvfrom(socket, mdesc->fragment[qp], MTU_LENGTH, 0, NULL, NULL);
	if (mdesc->fragment_size[qp] <= 0)
	{
		TL_ERROR("cannot read from socket");
		goto error;
	}

	int res = heap_push(mdesc, qp);
	if (res == -1)
	{
		TL_ERROR("cannot push to heap");
		goto error;
	}

	int hp = heap_pop(mdesc);
	if (hp == -1)
	{
		TL_ERROR("cannot pop from heap");
		goto error;
	}

	res = queue_push(mdesc, hp);
	if (res == -1)
	{
		TL_ERROR("cannot push to queue");
		goto error;
	}

	if (mdesc->frame_complete)
	{
		TL_WARNING("overwriting existing frame");
		mdesc->len = 0;
		mdesc->frame_complete = 0;
	}

	return mdesc->append(mdesc->fragment[hp], mdesc->fragment_size[hp], mdesc);

error:
	return -1;
}

int rtp_pop_frame(rtp_merge_desc* mdesc)
{
	//TL_INFO("frame_complete = %d", mdesc->frame_complete);
	if (mdesc->frame_complete)
	{
		mdesc->frame_complete = 0;
		mdesc->frame_lenght = mdesc->len;
		mdesc->len = 0;
		mdesc->ebit = 0;

		TL_DEBUG("frame size = %d", mdesc->frame_lenght);
		return 0;
	}
	return -1;
}
