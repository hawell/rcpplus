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
#include <arpa/inet.h>

#include "rtp.h"

unsigned char NAL_START_FRAME[] = {0x00, 0x00, 0x01};

int append_fu_a(nal_packet* np, fu_a_packet* fp, int len)
{
	if (fp->fuh.s == 1)
	{
		fprintf(stderr, "start\n");
		if (np->size == 0)
		{
			np->nh.f = fp->nh.f;
			np->nh.nri = fp->nh.nri;
			np->nh.type = fp->fuh.type;

			fprintf(stderr, "nal unit: f=%d nri=%d type=%d", np->nh.f, np->nh.nri, np->nh.type);

			memcpy(np->payload, fp->payload, len);
			np->size += len;
			return 0;
		}
		else
		{
			fprintf(stderr, "fragment unit is not the first unit\n");
			return -1;
		}
	}
	else
	{
		if (np->size == 0)
		{
			fprintf(stderr, "missing first fragment unit\n");
			return -1;
		}
		else
		{
			memcpy(np->payload+np->size, fp->payload, len);
			np->size += len;
		}
	}

	if (fp->fuh.e == 1)
	{
		fprintf(stderr, "end\n");
		return 1;
	}

	return 0;
}

int append_stap_a(nal_packet* np, unsigned char* data)
{
	data++;
	np[0].size = ntohs(*(unsigned short*)(data));
	np[0].nh = *(nal_unit_header*)(data+2);
	memcpy(np[0].payload, data+3, np[0].size);

	data += np[0].size + 2;
	np[1].size = ntohs(*(unsigned short*)(data));
	np[1].nh = *(nal_unit_header*)(data+2);
	memcpy(np[1].payload, data+3, np[1].size);

	return 2;
}

int defrag(nal_packet* np, void* buffer, int len)
{
	rtp_header *rtp = (rtp_header*)buffer;
	int rtp_header_len = RTP_HEADER_LENGTH_MANDATORY + rtp->cc*4;

	//fprintf(stderr, "v  - %d\np  - %d\nx  - %d\ncc - %d\nm  - %d\npt - %d\nsq - %d\n\n", rtp->version, rtp->p, rtp->x, rtp->cc, rtp->m, rtp->pt, rtp->seq);
	unsigned char nal_unit_header = *((unsigned char*)buffer + rtp_header_len);
	fprintf(stderr, "nal = %hhx\n", nal_unit_header);

	unsigned char nal_type = nal_unit_header & 0x1F;
	switch (nal_type)
	{
		case 24: // STAP-A
		{
			log_hex("STAP-A", buffer + rtp_header_len, len-rtp_header_len);
			return append_stap_a(np, buffer+rtp_header_len);
			//fprintf(stderr, "sps: %x %d\n", *(unsigned char*)&np[0].nh, np[0].size);
			//log_hex("payload", np[0].payload, np[0].size);
			//fprintf(stderr, "pps: %x %d\n", *(unsigned char*)&np[1].nh, np[1].size);
			//log_hex("payload", np[1].payload, np[1].size);
		}
		break;

		case 28: // FU-A
		{
			fu_a_packet *fp = (buffer + rtp_header_len);
			fprintf(stderr, "e=%d r=%d s=%d type=%d\n", fp->fuh.e, fp->fuh.r, fp->fuh.s, fp->fuh.type);
			return append_fu_a(np, fp, len - (rtp_header_len+FU_A_HEADER_LENGTH));
		}
		break;

		default:
			fprintf(stderr, "unsupported nal type\n");
			break;
	}

	return 0;
}
