/*
 * test_transcode.c
 *
 *  Created on: May 11, 2025
 *	  Author: eggor
 */

#if !defined(_GNU_SOURCE) /* for memmem(), strsignal(), etc etc... */
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <sysexits.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>

#include <tlog/tlog.h>

#include <rcpplus.h>
#include <rcpcommand.h>
#include <rcpdefs.h>
#include <caps.h>
#include <coder.h>
#include <preset.h>
#include <rtp.h>

static const char* out_filename = "t-coded.mkv";
static const char* out_format = "matroska";
static const enum AVCodecID av_codec_id = AV_CODEC_ID_H265;
static const int max_transcoding_time_s = 16;

typedef struct Context {
	rcp_session* session;
	rtp_merge_desc* merge_desc;
	AVFormatContext* in_fc;
	AVCodecContext* in_cc;
	AVCodecParserContext* pc;
	AVFormatContext* out_fc;
	AVCodecContext* out_cc;
	AVPacket* pkt;
	AVFrame* frame;
	int64_t pts;
	int64_t pts_inc;
	unsigned int in_stream_number;
	int is_rcpp;
	pthread_t keep_alive_thread;
} Context;

Context* init_context(const char* input, const char* pwd);
int transcode_stream(Context* c);
int transcode_file(Context* c);
int transcode_write(Context* c, AVPacket* enc_pkt);
int encode_write(const Context* c, AVFrame* dec_frame);
void free_context(Context** pc);
void handle_stop(const int sig);

#define ARRSIZ(arr) (sizeof(arr) / sizeof(*arr))
static volatile sig_atomic_t stop = 0;

int main(int argc, char* argv[])
{
	if (argc < 2) {
		printf("Usage: %s ip_addr\n", argv[0]);
		return EX_USAGE;
	}

	signal(SIGTERM, handle_stop);
	signal(SIGINT, handle_stop);

	const char* pwd = getenv("PWD");
	Context* c = init_context(argv[1], pwd ? pwd : "");

	if (!c) {
		fprintf(stderr, "No context\n");
		return EX_SOFTWARE;
	}

	int rc = 0;
	if ((rc = c->is_rcpp ? transcode_stream(c) : transcode_file(c)) < 0)
		fprintf(stderr, "Transcoding failed\n");

	if (transcode_write(c, NULL) < 0)
		fprintf(stderr, "Flash decoder failed\n");

	if (encode_write(c, NULL) < 0)
		fprintf(stderr, "Flash encoder failed\n");

	av_write_trailer(c->out_fc);

	free_context(&c);

	return rc;
}

Context* open_input_stream(const char* ip_addr, const char* pwd);
Context* open_input_file(const char* filename);
int open_output(Context* c, const char* filename, const char* format);

Context* init_context(const char* input, const char* pwd)
{
	int rc = 0;

	Context* c = open_input_stream(input, pwd);
	//Context* c = open_input_file(input);
	if (!c) {
		fprintf(stderr, "No input\n");
		exit(EX_NOINPUT);
	}

	if (open_output(c, out_filename, out_format) < 0) {
		rc = EX_CANTCREAT;
		goto fail;
	}

	c->pkt = av_packet_alloc();
	if (!c->pkt) {
		av_log(NULL, AV_LOG_ERROR, "av_packet_alloc() failed");
		rc = EX_OSERR;
		goto fail;
	}

	c->frame = av_frame_alloc();
	if (!c->frame) {
		av_log(NULL, AV_LOG_ERROR, "av_frame_alloc() failed");
		rc = EX_OSERR;
		goto fail;
	}

	c->pts_inc = av_rescale_q(1, av_inv_q(c->in_cc->framerate), c->in_cc->pkt_timebase);

	return c;

fail:
	free_context(&c);
	exit(rc);
}

int transcode_file(Context* c)
{
	int rc = 0;
	while (!stop && av_read_frame(c->in_fc, c->pkt) >= 0) {
		av_log(NULL, AV_LOG_DEBUG, ">p %"PRId64"\n", c->pkt->pts);

		if ((rc = transcode_write(c, c->pkt)) < 0)
			break;
	}

	return rc;
}

int transcode_stream(Context* c)
{
	int rc = 0;

	while (!stop) {
		if (rtp_recv(c->session->stream_socket, c->merge_desc) == 0
			&& rtp_pop_frame(c->merge_desc) == 0) {
			int pos = 0;

			while (c->merge_desc->frame_lenght > 0) {
				const int parsed = av_parser_parse2(c->pc, c->in_cc, &c->pkt->data, &c->pkt->size,
					c->merge_desc->data, c->merge_desc->frame_lenght, AV_NOPTS_VALUE, AV_NOPTS_VALUE,
					pos);

				if (parsed < 0) {
					av_log(NULL, AV_LOG_ERROR, "av_parser_parse2(): %s", av_err2str(parsed));
					return parsed;
				}

				pos += parsed;
				c->merge_desc->frame_lenght -= parsed;

				if (c->pkt->size > 0) {
					av_log(NULL, AV_LOG_DEBUG, ">p %"PRId64"\n", c->pkt->pts);
					if ((rc = transcode_write(c, c->pkt)) < 0)
						return rc;

					const int64_t t_coded_s = av_rescale(c->pts, c->in_cc->pkt_timebase.num,
						c->in_cc->pkt_timebase.den);

					if (t_coded_s > max_transcoding_time_s)
						return 0;
				}
			}
		}
	}

	return 0;
}

int transcode_write(Context* c, AVPacket* enc_pkt)
{
	int rc = 0;

	if ((rc = avcodec_send_packet(c->in_cc, enc_pkt)) < 0) {
		if (rc == AVERROR_EOF || rc == AVERROR(EAGAIN))
			return 0;
		av_log(NULL, AV_LOG_WARNING, "avcodec_send_packet(): %d (%s)\n",
			   rc, av_err2str(rc));

		if (rc == AVERROR_INVALIDDATA)
			rc = 0;
		else
			return rc;
	}

	while (rc >= 0) {
		if ((rc = avcodec_receive_frame(c->in_cc, c->frame)) < 0) {
			if (rc == AVERROR_EOF || rc == AVERROR(EAGAIN))
				break;

			if (rc < 0) {
				av_log(NULL, AV_LOG_WARNING, "avcodec_receive_frame(): %d (%s)\n",
					   rc, av_err2str(rc));
				return rc;
			}
		}

		if (c->frame->best_effort_timestamp == AV_NOPTS_VALUE) {
			c->frame->pts = c->pts;
			c->pts += c->pts_inc;
		}
		else
			c->frame->pts = c->frame->best_effort_timestamp;

		av_log(NULL, AV_LOG_DEBUG, ">f %" PRId64 "\n", c->frame->pts);

		if ((rc = encode_write(c, c->frame)) < 0)
			return rc;

		av_packet_unref(enc_pkt);
	}

	return 0;
}

int encode_write(const Context* c, AVFrame* dec_frame)
{
	int rc = 0;

	if (dec_frame)
		dec_frame->pts = av_rescale_q(dec_frame->pts, c->in_cc->pkt_timebase,
			c->out_cc->time_base);

	av_log(NULL, AV_LOG_DEBUG, "<f %" PRId64 "\n", dec_frame ? dec_frame->pts : 0);

	if ((rc = avcodec_send_frame(c->out_cc, dec_frame)) < 0) {
		if (rc == AVERROR(EAGAIN))
			return 0;

		if (rc < 0) {
			av_log(NULL, AV_LOG_ERROR, "avcodec_send_frame(): %d (%s)\n",
				   rc, av_err2str(rc));
			return rc;
		}
	}

	while (rc >= 0) {
		if ((rc = avcodec_receive_packet(c->out_cc, c->pkt)) < 0) {
			if (rc == AVERROR(EAGAIN) || rc == AVERROR_EOF)
				break;

			if (rc < 0) {
				av_log(NULL, AV_LOG_ERROR, "avcodec_receive_packet(): %d (%s)\n",
					   rc, av_err2str(rc));
				return rc;
			}
		}

		av_packet_rescale_ts(c->pkt, c->out_cc->time_base,
			c->out_fc->streams[0]->time_base);

		av_log(NULL, AV_LOG_DEBUG, "<p %" PRId64 "\n", c->pkt->pts);

		rc = av_interleaved_write_frame(c->out_fc, c->pkt);
	}

	return 0;
}

rcp_session* init_rcp_session(unsigned char proto, rcp_media_descriptor* md);
int get_video_resolution(const rcp_media_descriptor* md, int* width, int* height);
int init_media_descriptor(unsigned char proto, unsigned short port,
	rcp_media_descriptor* descr);
unsigned char* get_sps_pps(const rcp_session* session, int encoder, int* sps_pps_len);
void* keep_alive_thread(void *params);

Context* open_input_stream(const char* ip_addr, const char* pwd)
{
	tlog_init(TLOG_MODE_STDERR, TLOG_INFO, NULL);

	int rc = 0;

	if (rcp_connect(ip_addr) < 0)
		return NULL;

	start_message_manager();

	if (client_register(RCP_USER_LEVEL_LIVE, pwd, RCP_REGISTRATION_TYPE_NORMAL,
		RCP_ENCRYPTION_MODE_PLAIN) < 0) {
		fprintf(stderr, "Wrong password?\n");
		return NULL;
	}

	rcp_media_descriptor md;
	rcp_session* session = init_rcp_session(RCP_MEP_UDP, &md);
	if (!session) {
		TL_ERROR("Cannot initialize RCP sesssion");
		return NULL;
	}

	int width = 0, height = 0;
	if (get_video_resolution(&md, &width, &height) < 0)
		goto fail1;

	rtp_merge_desc* merge_desc = malloc(sizeof(rtp_merge_desc));
	if (!merge_desc) {
		TL_ERROR("No %zu bytes of free memory", sizeof(rtp_merge_desc));
		goto fail1;
	}

	const int pload_type = md.coding == RCP_VIDEO_CODING_H264 ? RTP_PAYLOAD_TYPE_H264 : RTP_PAYLOAD_TYPE_H263;
	rtp_init(pload_type, 1, merge_desc);

	const int av_codec_id = md.coding == RCP_VIDEO_CODING_H264 ? AV_CODEC_ID_H264 : AV_CODEC_ID_H263;
	const AVCodec* dec = avcodec_find_decoder(av_codec_id);
	if (dec == NULL) {
		av_log(NULL, AV_LOG_ERROR, "avcodec_find_decoder(%d)\n", av_codec_id);
		goto fail2;
	}

	AVCodecContext* cc = avcodec_alloc_context3(dec);
	if (cc == NULL) {
		av_log(NULL, AV_LOG_ERROR, "avcodec_alloc_context3() failed\n");
		goto fail2;
	}

	AVCodecParserContext* pc = av_parser_init(dec->id);
	if (!pc) {
		av_log(NULL, AV_LOG_ERROR, "av_parser_init() failed\n");
		goto fail3;
	}

	cc->width = width;
	cc->height = height;
	cc->pix_fmt = AV_PIX_FMT_YUV420P;
	cc->framerate = (AVRational){25, 1};
	cc->pkt_timebase = (AVRational){1, 1200000};

	if (md.coding == RCP_VIDEO_CODING_H264) {
		int sps_pps_len;
		unsigned char* sps_pps = get_sps_pps(session, md.coder, &sps_pps_len);
		cc->extradata = sps_pps;
		cc->extradata_size = sps_pps_len;
	}

	pthread_t thread;
	if ((rc = pthread_create(&thread, NULL, keep_alive_thread, session)) != 0) {
		TL_ERROR("pthread_create(): %s", strerror(rc));
		goto fail4;
	}

	if ((rc = avcodec_open2(cc, dec, NULL)) < 0) {
		av_log(NULL, AV_LOG_ERROR, "avcodec_open2(%s): %s\n",
			dec->name ? dec->name : "???", av_err2str(rc));
		goto fail5;
	}

	AVFormatContext* fc = NULL;

	Context* c = malloc(sizeof(Context));
	if (!c) {
		av_log(NULL, AV_LOG_ERROR, "No %zu bytes of free memory\n", sizeof(Context));
		goto fail5;
	}

	memset(c, 0, sizeof(Context));

	c->session = session;
	c->merge_desc = merge_desc;
	c->pc = pc;
	c->in_fc = fc;
	c->in_cc = cc;
	c->in_stream_number = 0;
	c->keep_alive_thread = thread;
	c->is_rcpp = 1;

	return c;

fail5:
	pthread_cancel(thread);
fail4:
	av_parser_close(pc);
fail3:
	avcodec_free_context(&cc);
fail2:
	free(merge_desc);
fail1:
	free(session);

	return NULL;
}

Context* open_input_file(const char* filename)
{
	AVFormatContext* fc = NULL;
	int rc = 0;

	if ((rc = avformat_open_input(&fc, filename, NULL, NULL)) < 0) {
		av_log(NULL, AV_LOG_ERROR, "avformat_open_input(%s): %s\n",
			filename, av_err2str(rc));
		return NULL;
	}

	if ((rc = avformat_find_stream_info(fc, NULL)) < 0) {
		av_log(NULL, AV_LOG_ERROR, "avformat_find_stream_info(): %s\n",
			av_err2str(rc));
		goto fail1;
	}

	AVCodecContext* cc = NULL;

	unsigned int stream_number = UINT_MAX;
	for (unsigned int i = 0; i < fc->nb_streams; ++i) {
		AVStream* stream = fc->streams[i];
		const AVCodec* dec = avcodec_find_decoder(stream->codecpar->codec_id);
		if (!dec) {
			av_log(NULL, AV_LOG_ERROR, "avcodec_find_decoder(): %s\n",
				   av_err2str(rc));
			goto fail1;
		}

		if (dec->type == AVMEDIA_TYPE_VIDEO) {
			stream_number = i;
			cc = avcodec_alloc_context3(dec);

			if (!cc) {
				av_log(NULL, AV_LOG_ERROR, "avcodec_alloc_context3()\n");
				goto fail1;
			}

			if ((rc = avcodec_parameters_to_context(cc, stream->codecpar)) < 0) {
				av_log(NULL, AV_LOG_ERROR, "avcodec_parameters_to_context(): %s\n",
					   av_err2str(rc));
				goto fail2;
			};

			cc->pkt_timebase = stream->time_base;
			cc->framerate = av_guess_frame_rate(fc, stream, NULL);

			if ((rc = avcodec_open2(cc, dec, NULL)) < 0) {
				av_log(NULL, AV_LOG_ERROR, "avcodec_open2(%s): %s\n",
					dec->name ? dec->name : "???", av_err2str(rc));
				goto fail2;
			}

			break;
		}

		av_log(NULL, AV_LOG_INFO, "skip %d stream\n", dec->type);
	}

	if (stream_number == UINT_MAX) {
		av_log(NULL, AV_LOG_ERROR, "No video steam found\n");
		goto fail2;
	}

	Context* c = malloc(sizeof(Context));
	if (!c) {
		av_log(NULL, AV_LOG_ERROR, "No %zu bytes of free memory\n",
			   sizeof(Context));
		goto fail2;
	}

	memset(c, 0, sizeof(Context));

	c->in_fc = fc;
	c->in_cc = cc;
	c->in_stream_number = stream_number;

	av_dump_format(fc, 0, filename, 0);

	return c;

fail2:
	avcodec_free_context(&cc);
fail1:
	avformat_close_input(&fc);

	return NULL;
}

int open_output(Context* c, const char* filename, const char* format)
{
	int rc;

	AVFormatContext* fc = NULL;
	if ((rc = avformat_alloc_output_context2(&fc, NULL, format, filename)) < 0) {
		av_log(NULL, AV_LOG_ERROR, "avformat_alloc_output_context2(): %s\n",
			av_err2str(rc));
		return rc;
	}

	if (!fc) {
		av_log(NULL, AV_LOG_ERROR, "avformat_alloc_output_context2(): %s\n",
			av_err2str(rc));
		return AVERROR(ENOMEM);
	}

	AVStream* stream_out = avformat_new_stream(fc, NULL);
	if (!stream_out) {
		av_log(NULL, AV_LOG_ERROR, "avformat_new_stream(): %s\n",
			av_err2str(rc));
		return AVERROR(ENOMEM);
	}

	const AVCodecContext* dec_cc = c->in_cc;
	const enum AVCodecID wanted_codec_id = av_codec_id ? av_codec_id : dec_cc->codec_id;
	const AVCodec* enc = avcodec_find_encoder(wanted_codec_id);
	if (!enc) {
		av_log(NULL, AV_LOG_ERROR, "avcodec_find_encoder(%d) failed\n", wanted_codec_id);
		return AVERROR(EX_UNAVAILABLE);
	}

	AVCodecContext* enc_cc = avcodec_alloc_context3(enc);
	if (!enc_cc) {
		av_log(NULL, AV_LOG_ERROR, "avcodec_alloc_context3()\n");
		return AVERROR(ENOMEM);
	}

	const enum AVPixelFormat* pix_fmts = NULL;

	enc_cc->height = dec_cc->height;
	enc_cc->width = dec_cc->width;
	enc_cc->sample_aspect_ratio = dec_cc->sample_aspect_ratio;

	#if LIBAVFORMAT_VERSION_MAJOR > 60

	if ((rc = avcodec_get_supported_config(dec_cc, NULL, AV_CODEC_CONFIG_PIX_FORMAT,
		0, (const void**)&pix_fmts, NULL)) < 0) {
		av_log(NULL, AV_LOG_WARNING, "avcodec_get_supported_config(): %s\n",
			   av_err2str(rc));
	}

	#endif // LIBAVFORMAT_VERSION_MAJOR > 60

	enc_cc->pix_fmt = rc >= 0 && pix_fmts ? pix_fmts[0] : dec_cc->pix_fmt;
	enc_cc->time_base = av_inv_q(dec_cc->framerate);
	enc_cc->color_range = AVCOL_RANGE_JPEG;

	if (fc->oformat->flags & AVFMT_GLOBALHEADER)
		enc_cc->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

	if ((rc = avcodec_open2(enc_cc, enc, NULL)) < 0) {
		av_log(NULL, AV_LOG_ERROR, "avcodec_open2(%s): %s\n",
			enc->name ? enc->name : "???", av_err2str(rc));
		return rc;
	}

	if ((rc = avcodec_parameters_from_context(stream_out->codecpar, enc_cc)) < 0) {
		av_log(NULL, AV_LOG_ERROR, "avcodec_parameters_from_context(): %s\n",
			   av_err2str(rc));
		return rc;
	}

	stream_out->time_base = enc_cc->time_base;

	if (!(fc->oformat->flags & AVFMT_NOFILE)
		&& (rc = avio_open(&fc->pb, filename, AVIO_FLAG_WRITE)) < 0) {
		av_log(NULL, AV_LOG_ERROR, "avio_open(): %s\n",
			   av_err2str(rc));
		return rc;
	}

	if ((rc = avformat_write_header(fc, NULL)) < 0) {
		av_log(NULL, AV_LOG_ERROR, "avformat_write_header(): %s\n",
			   av_err2str(rc));
		return rc;
	}

	c->out_fc = fc;
	c->out_cc = enc_cc;
	c->in_stream_number = 0;

	av_dump_format(fc, 0, filename, 1);

	return 0;
}

void free_context(Context** pc)
{
	Context* c = *pc;

	pthread_cancel(c->keep_alive_thread);

	av_parser_close(c->pc);
	free(c->merge_desc);
	free(c->session);

	av_frame_free(&c->frame);
	av_packet_free(&c->pkt);

	avcodec_free_context(&c->out_cc);
	avformat_free_context(c->out_fc);

	avcodec_free_context(&c->in_cc);
	avformat_close_input(&c->in_fc);

	free(c);
}

rcp_session* init_rcp_session(const unsigned char proto, rcp_media_descriptor* md)
{
	rcp_session* session = malloc(sizeof(rcp_session));
	if (!session) {
		TL_ERROR("No %zu bytes of free memory", sizeof(rcp_session));
		return NULL;
	}

	memset(session, 0, sizeof(rcp_session));
	const unsigned short port = stream_connect_udp(session);

	if (init_media_descriptor(proto, port, md) < 0) {
		TL_ERROR("Unable to init media descriptor");
		goto fail;
	}

	if (client_connect(session, RCP_CONNECTION_METHOD_GET,
		RCP_MEDIA_TYPE_VIDEO, 0, md) < 0)
		goto fail;

	return session;

fail:
	free(session);

	return NULL;
}

int init_media_descriptor(const unsigned char proto, const unsigned short port,
	rcp_media_descriptor* descr)
{
	cap_list caps;
	get_capability_list(&caps);

	int video_sect_num = 0;
	for (; video_sect_num < caps.section_count; ++video_sect_num)
		if (caps.sections[video_sect_num].section_type == CAP_SECTION_TYPE_VIDEO)
			break;

	if (video_sect_num == caps.section_count
		|| caps.sections[video_sect_num].section_type != CAP_SECTION_TYPE_VIDEO) {
		TL_ERROR("No video section found");
		return -1;
	}

	const cap_section* section_video = &caps.sections[video_sect_num];

	int encoder = -1;
	int input_number = -1;
	int encoding = -1;
	int resolution = -1;

	for (int i = 0; i < section_video->n_elems; i++) {
		const cap_element* cap = &section_video->elements[i];

		if (cap && cap->element_type == CAP_ELEMENT_VIDEO_ENCODER) {
			for (int j = 1; j <= 0x8000; j <<= 1)
				if (cap->resolution & j && j > resolution) {
					resolution = j;
					input_number = cap->input_number;
					encoder = cap->identifier;

					if (cap->compression & VIDEO_COMP_H264)
						encoding = RCP_VIDEO_CODING_H264;
					else if (cap->compression & VIDEO_COMP_MPEG4)
						encoding = RCP_VIDEO_CODING_MPEG4;
					else if (cap->compression & VIDEO_COMP_MPEG2)
						encoding = RCP_VIDEO_CODING_MPEG2S;
					else if (cap->compression & VIDEO_COMP_JPEG)
						encoding = RCP_VIDEO_CODING_JPEG;
					else
						encoding = RCP_VIDEO_CODING_H263;
				}
		}
		else {
			TL_WARNING("Capability [%d] is NULL\n", i);
		}
	}

	if (encoder == -1 || input_number == -1 || encoding == -1 || resolution == -1) {
		TL_ERROR("capability_id == %d, input_number == %d, encoding == %d, resolution == %d");
		return -1;
	}

	TL_DEBUG("capability_id == %d, input_number == %d, encoding == %d, resolution == %d");

	descr->encapsulation_protocol = proto;
	descr->substitude_connection = 1;
	descr->relative_addressing = 1;
	descr->mta_ip = 0;
	descr->mta_port = port;
	descr->coder = encoder;
	descr->line = input_number;
	descr->coding = encoding;
	descr->resolution = resolution;

	return 0;
}

int get_video_resolution(const rcp_media_descriptor* md, int* width, int* height)
{
	const char* resolution_name;

	int op_mode = -1;
	if (get_coder_video_operation_mode(md->coder, &op_mode) < 0) {
		TL_ERROR("get_coder_video_operation_mode() failed");
		return -1;
	}

	switch (op_mode) {
	case -1: {
			TL_ERROR("video_operation_mode ==-1");
			return -1;
		}
	case RCP_CODER_MODE_H264: {
			int h264_modes[2];
			if (get_h264_encoder_video_operation_mode(h264_modes) < 0) {
				TL_ERROR("get_h264_encoder_video_operation_mode() failed");
				return -1;
			}

			int h264_width = 0, h264_height = 0;
			for (size_t i = 0; i < ARRSIZ(h264_modes); ++i) {
				if (get_resolution_from_h264_operation_mode(h264_modes[i],
					&h264_width, &h264_height, &resolution_name) < 0) {
					TL_DEBUG("get_resolution_from_h264_operation_mode(%d) failed", i);
				}
				else if (h264_width * h264_height > *width * *height) {
						*width = h264_width;
						*height = h264_height;
				}
			}
			break;
		}
	default: {
			// Do nothing
		}
	}

	if (*width == 0 || *height == 0) {
		const int preset_id = get_coder_preset(md->coder);
		if (preset_id < 0)
			return -1;

		rcp_mpeg4_preset preset;
		memset(&preset, 0, sizeof preset);
		get_preset(preset_id, &preset, 1);
		log_preset(TLOG_DEBUG, &preset, 1);

		int format;
		if (get_video_input_format(md->line, &format) < 0)
			return -1;

		if (get_resolution_from_preset(&preset, format, width, height, &resolution_name) < 0)
			return -1;
	}

	return 0;
}

static unsigned char NAL_START_CODE_SHORT[] = {0x00, 0x00, 0x01};
static unsigned char NAL_START_CODE_LONG[] = {0x00, 0x00, 0x00, 0x01};

const char* find_sps(const char* sps_pps, int data_len, int* sps_len);
const char* find_pps(const char* sps_pps, int data_len, int* pps_len);

unsigned char* get_sps_pps(const rcp_session* session, const int encoder, int* sps_pps_len)
{
	char in_sps_pps[4096];
	int in_sps_pps_len;
	if (request_sps_pps(session, encoder, in_sps_pps, &in_sps_pps_len) < 0)
		return NULL;

	int sps_len;
	const char* sps = find_sps(in_sps_pps, in_sps_pps_len, &sps_len);

	int pps_len;
	const char* pps = find_pps(in_sps_pps, in_sps_pps_len, &pps_len);

	*sps_pps_len = (sps_len ? sps_len + ARRSIZ(NAL_START_CODE_LONG) : 0)
		+ (pps_len ? pps_len + ARRSIZ(NAL_START_CODE_LONG) : 0);

	unsigned char* normalised_sps_pps = NULL;

	if (*sps_pps_len > 0) {
		const ssize_t buf_size = *sps_pps_len + AV_INPUT_BUFFER_PADDING_SIZE;
		normalised_sps_pps = av_malloc(buf_size);
		memset(normalised_sps_pps, 0, buf_size);

		ssize_t ed_offset = 0;
		if (sps_len) {
			memcpy(normalised_sps_pps, NAL_START_CODE_LONG, ARRSIZ(NAL_START_CODE_LONG));
			ed_offset += ARRSIZ(NAL_START_CODE_LONG);
			memcpy(normalised_sps_pps + ed_offset, sps, sps_len);
			ed_offset += sps_len;
		}

		if (pps_len) {
			memcpy(normalised_sps_pps + ed_offset, NAL_START_CODE_LONG, ARRSIZ(NAL_START_CODE_LONG));
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

	if (sps_start) {
		const char* sps_end = memmem(sps_start + 1, data_len - (sps_start + 1 - sps_pps),
			NAL_START_CODE_LONG, ARRSIZ(NAL_START_CODE_LONG) - 1);

		if (sps_end)
			*sps_len = sps_end - sps_start;
		else {
			const char* sps_end = memmem(sps_start + 1, data_len - (sps_start + 1 - sps_pps),
				NAL_START_CODE_SHORT, ARRSIZ(NAL_START_CODE_SHORT));

			if (sps_end)
				*sps_len = sps_end - sps_start;
			else {
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
		*pps_len = 0;
	}

	return pps_start;
}

void* keep_alive_thread(void *params)
{
	rcp_session* session = params;
	int n;
	while ((n = keep_alive(session)) >= 0)
	{
		TL_DEBUG("active connections = %d", n);
		sleep(2);
	}

	return NULL;
}

void handle_stop(const int sig __attribute__((__unused__)))
{
	stop = 1;
}
