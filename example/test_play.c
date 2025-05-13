/*
 * test_play.c
 *
 *  Created on: Nov 7, 2012
 *      Author: arash
 */

#if !defined(_GNU_SOURCE) /* for memmem(), strsignal(), etc etc... */
#define _GNU_SOURCE
#endif

#define ARRSIZ(arr) (sizeof(arr) / sizeof(*arr))

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#include <sys/time.h>
#include <sys/types.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_thread.h>

#include <libavutil/avutil.h>
#include <libavcodec/avcodec.h>

#include <tlog/tlog.h>

#include "caps.h"
#include "coder.h"
#include "preset.h"
#include "rcpcommand.h"
#include "rcpdefs.h"
#include "rcpplus.h"
#include "rtp.h"

#define VIDEO_MODE_H263 1
#define VIDEO_MODE_H264 2

unsigned int highest_bit(unsigned int a);
unsigned char* get_sps_pps(const rcp_session* session, int encoder,
	int *sps_pps_len);
void* keep_alive_thread(void* params);

pthread_t thread_keep_alive;

int main(int argc, char *argv[])
{
	tlog_init(TLOG_MODE_STDERR, TLOG_INFO, NULL);

	if (argc < 2) {
		TL_INFO("%s ip [line] [coder]", argv[0]);
		return -1;
	}

	if (SDL_Init(SDL_INIT_VIDEO)) {
		TL_ERROR("could not initialize SDL - %s", SDL_GetError());
		return -1;
	}

	int line = 1;
	if (argc >= 3)
		line = atoi(argv[2]);

	int rc = 0;

	if ((rc = rcp_connect(argv[1])) < 0) {
		TL_ERROR("rcp_connect() error");
		return rc;
	}

	start_message_manager();

	const char* pwd = getenv("PWD");

	if ((rc = client_register(RCP_USER_LEVEL_SERVICE, pwd ? pwd : "",
		RCP_REGISTRATION_TYPE_NORMAL, RCP_ENCRYPTION_MODE_PLAIN)) < 0) {
		fprintf(stderr, "Wrong password?\n");
		return rc;
	}

	rcp_session session;
	memset(&session, 0, sizeof(rcp_session));

	const unsigned short udp_port = stream_connect_udp(&session);

	cap_list caps;
	get_capability_list(&caps);
	log_capabilities(TLOG_INFO, &caps);

	int video_section = 0;

	while (caps.sections[video_section].section_type != CAP_SECTION_TYPE_VIDEO)
		++video_section;

	int coder = -1, coding = -1;
	cap_section* section_video = &caps.sections[video_section];

	for (int i = 0; i < section_video->n_elems; i++) {
		cap_element *el = &section_video->elements[i];

		if (el) {
			if (el->element_type == CAP_ELEMENT_VIDEO_ENCODER &&
				el->input_number == line) {
				coder = el->identifier;
				coding = (el->compression & VIDEO_COMP_H264)
					? RCP_VIDEO_CODING_H264 : RCP_VIDEO_CODING_H263;
				break;
			}
		}
		else
			TL_INFO("Element [%d] is NULL\n", i);
	}

	if (argc >= 4)
		coder = atoi(argv[3]);

	if (coder == -1) {
		TL_ERROR("no coder found for selected line");
		goto end1;
	}

	int preset_id = get_coder_preset(coder);

	rcp_mpeg4_preset preset;
	memset(&preset, 0, sizeof preset);

	get_preset(preset_id, &preset, 1);
	log_preset(TLOG_INFO, &preset, 1);

	int resolution = 0x1EF; // we accept all resolutions

	rcp_media_descriptor desc = {
		RCP_MEP_UDP, 1, 1,
		0, udp_port, coder, line, coding, resolution
	};

	if (client_connect(&session, RCP_CONNECTION_METHOD_GET,
		RCP_MEDIA_TYPE_VIDEO, 0, &desc) != 0)
		return -1;

	int mode = -1;
	get_coder_video_operation_mode(coder, &mode);
	TL_INFO("mode = %d coder = %d", mode, coder);

	int format;
	get_video_input_format(1, &format);

	const char* res_name;
	int modes[2];
	int width, height;

	if (mode == RCP_CODER_MODE_H264 &&
		get_h264_encoder_video_operation_mode(modes) == 0 &&
		get_resolution_from_h264_operation_mode(modes[0],
			&width, &height, &res_name) != 0) {
		TL_INFO("preset resolution == %d", preset.resolution);
		get_resolution_from_preset(&preset, format, &width, &height, &res_name);
	}
	else {
		resolution = highest_bit(desc.resolution);
		get_resolution_from_preset(&preset, format, &width, &height, &res_name);
	}

	TL_INFO("width=%d height=%d %s", width, height, res_name);

	SDL_Window* window = SDL_CreateWindow("RCP+ player",
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		width, height, SDL_WINDOW_RESIZABLE);

	if (!window) {
		fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
		SDL_Quit();
		return -1;
	}

	SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	if (!renderer) {
		fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
		SDL_DestroyWindow(window);
		SDL_Quit();
		return -1;
	}

	SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_YV12,
		SDL_TEXTUREACCESS_STREAMING, width, height);

	if (!texture) {
		fprintf(stderr, "SDL_CreateTexture failed: %s\n", SDL_GetError());
		SDL_DestroyRenderer(renderer);
		SDL_DestroyWindow(window);
		SDL_Quit();
		return -1;
	}

	rtp_merge_desc mdesc;
	const int pload_type = coding == RCP_VIDEO_CODING_H264 ? RTP_PAYLOAD_TYPE_H264 : RTP_PAYLOAD_TYPE_H263;
	if (rtp_init(pload_type, 1, &mdesc) < 0) {
		TL_ERROR("rtp_init()");
		return -1;
	}

	const enum AVCodecID codec_id = coding == RCP_VIDEO_CODING_H264 ? AV_CODEC_ID_H264 : AV_CODEC_ID_H263;
	const AVCodec* decoder = avcodec_find_decoder(codec_id);
	if (decoder == NULL) {
		TL_ERROR("cannot find decoder");
		return -1;
	}
	
	AVCodecContext* decoder_ctx = avcodec_alloc_context3(decoder);
	if (decoder_ctx == NULL) {
		TL_ERROR("cannot allocate decoder context");
		return -1;
	}
	
	AVCodecParserContext* parser = av_parser_init(decoder->id);
	if (!parser) {
		TL_ERROR("cannot init parser");
		return -1;
	}
	
	if (avcodec_open2(decoder_ctx, decoder, NULL) < 0) {
		TL_ERROR("cannot open codec");
		return -1;
	}
	
	decoder_ctx->width = width;
	decoder_ctx->height = height;
	decoder_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
	
	//***********************************************
	
	pthread_create(&thread_keep_alive, NULL, keep_alive_thread, &session);
	
	/*FILE* fout = fopen("stream.h264", "w");
	
	if (!fout) {
		TL_ERROR("cannot open output file");
		return -1;
	}*/

	int sps_pps_len;
	unsigned char* sps_pps = get_sps_pps(&session, coder, &sps_pps_len);
	if (sps_pps) {
		// fwrite(sps_pps, 1, sps_pps_len, fout);
		decoder_ctx->extradata = sps_pps;
		decoder_ctx->extradata_size = sps_pps_len;
	}
	
	SDL_Event event = {0};
	AVPacket pkt = {0};
	AVFrame frame = {0};
	
	while (event.type != SDL_QUIT) {
		SDL_PollEvent(&event);
		if (rtp_recv(session.stream_socket, &mdesc) == 0 &&
			rtp_pop_frame(&mdesc) == 0) {
			// fwrite(mdesc.data, 1, mdesc.frame_lenght, fout);
			int pos = 0;

			while (mdesc.frame_lenght > 0) {
				const int parsed = av_parser_parse2(parser, decoder_ctx,
					&pkt.data, &pkt.size, mdesc.data, mdesc.frame_lenght,
					AV_NOPTS_VALUE, AV_NOPTS_VALUE, pos);

				if (parsed < 0) {
					TL_WARNING("Parse error %s", av_err2str(parsed));
					break;
				}

				pos += parsed;
				mdesc.frame_lenght -= parsed;

				if (pkt.size > 0 && avcodec_send_packet(decoder_ctx, &pkt) == 0)
					while (avcodec_receive_frame(decoder_ctx, &frame) == 0) {
						SDL_UpdateYUVTexture(texture, NULL, frame.data[0], frame.linesize[0],
							frame.data[1], frame.linesize[1], frame.data[2], frame.linesize[2]);
						SDL_RenderClear(renderer);
						SDL_RenderCopy(renderer, texture, NULL, NULL);
						SDL_RenderPresent(renderer);
						SDL_PollEvent(&event);
					}
			}
		}
	}
	
	// fclose(fout);
	pthread_cancel(thread_keep_alive);
	client_disconnect(&session);
	
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	
	av_parser_close(parser);
	avcodec_free_context(&decoder_ctx);

end1:
	client_unregister();
	stop_message_manager();
	
	SDL_Quit();

	return 0;
}

void save_frame(const AVFrame* frame, const int width, const int height)
{
	static int fnum = 0;
	char filename[32];

	// Open file
	sprintf(filename, "frame%02d.ppm", fnum++);
	FILE* f = fopen(filename, "wb");
	if (f == NULL)
		return;

	// Write header
	fprintf(f, "P6\n%d %d\n255\n", width, height);

	// Write pixel data
	for (int y = 0; y < height; y++)
		fwrite(frame->data[0] + y * frame->linesize[0], 1, width * 3, f);

	// Close file
	fclose(f);
}

void* keep_alive_thread(void* params)
{
	rcp_session* session = params;
	while (1) {
		const int n = keep_alive(session);
		TL_DEBUG("active connections = %d", n);
		if (n < 0)
			break;

		sleep(2);
	}

	return NULL;
}

unsigned int highest_bit(unsigned int a)
{
	if (!a)
		return 0;

	unsigned int r = 1;
	while (a >>= 1)
		r <<= 1;

	return r;
}

static unsigned char NAL_START_CODE_SHORT[] = {0x00, 0x00, 0x01};
static unsigned char NAL_START_CODE_LONG[] = {0x00, 0x00, 0x00, 0x01};

const char* find_sps(const char* sps_pps, int data_len, int* sps_len);
const char* find_pps(const char* sps_pps, int data_len, int* pps_len);

unsigned char* get_sps_pps(const rcp_session* session, const int encoder,
	int* sps_pps_len)
{
	char in_sps_pps[4096];
	int in_sps_pps_len;
	if (request_sps_pps(session, encoder, in_sps_pps, &in_sps_pps_len) < 0)
		return NULL;

	int sps_len;
	const char* sps = find_sps(in_sps_pps, in_sps_pps_len, &sps_len);

	int pps_len;
	const char* pps = find_pps(in_sps_pps, in_sps_pps_len, &pps_len);

	*sps_pps_len = (sps_len ? sps_len + ARRSIZ(NAL_START_CODE_LONG) : 0) +
				   (pps_len ? pps_len + ARRSIZ(NAL_START_CODE_LONG) : 0);

	unsigned char* normalised_sps_pps = NULL;

	if (*sps_pps_len > 0)
	{
		const ssize_t buf_size = *sps_pps_len + AV_INPUT_BUFFER_PADDING_SIZE;
		normalised_sps_pps = av_malloc(buf_size);
		memset(normalised_sps_pps, 0, buf_size);

		ssize_t ed_offset = 0;
		if (sps_len) {
			memcpy(normalised_sps_pps, NAL_START_CODE_LONG,
				   ARRSIZ(NAL_START_CODE_LONG));
			ed_offset += ARRSIZ(NAL_START_CODE_LONG);
			memcpy(normalised_sps_pps + ed_offset, sps, sps_len);
			ed_offset += sps_len;
		}

		if (pps_len) {
			memcpy(normalised_sps_pps + ed_offset, NAL_START_CODE_LONG,
				   ARRSIZ(NAL_START_CODE_LONG));
			ed_offset += ARRSIZ(NAL_START_CODE_LONG);
			memcpy(normalised_sps_pps + ed_offset, pps, pps_len);
		}
	}

	return normalised_sps_pps;
}

const char* find_sps(const char* sps_pps, const int data_len, int* sps_len)
{
	static const char sps[] = {0x67};
	static const char pps[] = {0x68};
	const char* sps_start = memmem(sps_pps, data_len, sps, ARRSIZ(sps));

	if (sps_start)
	{
		const char* sps_end = memmem(sps_start + 1, data_len - (sps_start + 1 - sps_pps),
			NAL_START_CODE_LONG, ARRSIZ(NAL_START_CODE_LONG) - 1);

		if (sps_end)
			*sps_len = sps_end - sps_start;
		else
		{
			const char* sps_end = memmem(sps_start + 1, data_len - (sps_start + 1 - sps_pps),
				NAL_START_CODE_SHORT, ARRSIZ(NAL_START_CODE_SHORT));

			if (sps_end)
				*sps_len = sps_end - sps_start;
			else
			{
				const char* pps_pos = memmem(sps_start + 1, data_len - (sps_start + 1 - sps_pps),
					pps, ARRSIZ(pps));
				*sps_len = pps_pos ? pps_pos - sps_start : data_len - (sps_start - sps_pps);
			}
		}

		TL_DEBUG("Found sps %d bytes at offset %d", *sps_len, sps_start - sps_pps);
		tlog_hex(TLOG_DEBUG, "sps", sps_start, *sps_len);
	}
	else {
		TL_WARNING("sps not found");
		*sps_len = 0;
	}

	return sps_start;
}

const char* find_pps(const char* sps_pps, const int data_len, int* pps_len)
{
	static const char pps[] = {0x68};
	const char* pps_start = memmem(sps_pps, data_len, pps, ARRSIZ(pps));

	if (pps_start) {
		const char* pps_end = memmem(pps_start + 1, data_len - (pps_start + 1 - sps_pps),
			NAL_START_CODE_LONG, ARRSIZ(NAL_START_CODE_LONG) - 1);

		if (pps_end)
			*pps_len = pps_end - pps_start;
		else {
			const char* pps_end = memmem(pps_start + 1, data_len - (pps_start + 1 - sps_pps),
				NAL_START_CODE_SHORT, ARRSIZ(NAL_START_CODE_SHORT));

			if (pps_end)
				*pps_len = pps_end - pps_start;
			else
				*pps_len = data_len - (pps_start - sps_pps);
		}

		TL_DEBUG("Found pps %d bytes at offset %d", *pps_len, pps_start - sps_pps);
		tlog_hex(TLOG_DEBUG, "pps", pps_start, *pps_len);
	}
	else {
		TL_WARNING("pps not found");
	}

	return pps_start;
}
