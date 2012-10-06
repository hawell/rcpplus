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

#define MAX_FRAME_LENGTH	200000

#define RTP_PAYLOAD_TYPE_H263	1
#define RTP_PAYLOAD_TYPE_H264	2

typedef struct rtp_merge_desc {
	int type;
	unsigned char data[MAX_FRAME_LENGTH];
	int len;
	int timestamp;
	int frame_complete;
	int frame_error;
	int (*append)(unsigned char* fragment, int frag_len, struct rtp_merge_desc* mdesc);

} rtp_merge_desc;

typedef struct {
	unsigned char* data;
	int len;
	int timestamp;
} video_frame;

int rtp_init(int type, rtp_merge_desc* mdesc);

int rtp_push_frame(unsigned char* frame, int frame_size, rtp_merge_desc* mdesc);
int rtp_pop_frame(video_frame* frame, rtp_merge_desc* mdesc);

#endif /* RTP_H_ */
