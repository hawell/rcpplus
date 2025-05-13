/*
 * test_audio.c
 *
 *  Created on: Oct 20, 2013
 *      Author: arash
 */

#include <pthread.h>
#include <sys/time.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_thread.h>
#include <SDL2/SDL_audio.h>

#include <tlog/tlog.h>

#include "rcpdefs.h"
#include "rcpplus.h"
#include "rcpcommand.h"
#include "coder.h"
#include "audio.h"

static const enum AVCodecID codec_id = AV_CODEC_ID_PCM_MULAW;
static volatile sig_atomic_t stop = 0;

void handle_stop(const int sig __attribute__((__unused__)))
{
	stop = 1;
}

pthread_t thread;
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

#define BUF_SIZE 1024

int main(int argc, char* argv[])
{
	tlog_init(TLOG_MODE_STDERR, TLOG_INFO, NULL);

	int rc = 0;

	if (argc < 2) {
		TL_INFO("%s <ip>\n", argv[0]);
		return EX_USAGE;
	}

	signal(SIGTERM, handle_stop);
	signal(SIGINT, handle_stop);

	if ((rc = rcp_connect(argv[1])) < 0)
		return rc;

	start_message_manager();

	const char* pwd = getenv("PWD");

	if ((rc = client_register(RCP_USER_LEVEL_LIVE, pwd ? pwd : "",
		RCP_REGISTRATION_TYPE_NORMAL, RCP_ENCRYPTION_MODE_PLAIN)) < 0) {
			fprintf(stderr, "Wrong password?\n");
		return rc;
	}

	int max_in = get_max_audio_input_level(1);
	set_audio_input_level(1, max_in);

	if (get_audio_input(1) != RCP_AUDIO_INPUT_MIC)
		set_audio_input(1, RCP_AUDIO_INPUT_MIC);

	int max_mic = get_max_mic_level(1);
	set_mic_level(1, max_mic);

	rcp_coder_list encoders;
	get_coder_list(RCP_CODER_DECODER, RCP_MEDIA_TYPE_AUDIO, &encoders, 1);

	TL_INFO("-----------------------");

	for (int i = 0; i < encoders.count; i++) {
		TL_INFO("%x %x %x %x %x", encoders.coder[i].number, encoders.coder[i].caps, encoders.coder[i].current_cap,
		        encoders.coder[i].param_caps, encoders.coder[i].current_param);
		log_coder(TLOG_INFO, &encoders.coder[i]);
		TL_INFO("-----------------------");
	}
	//int coder_id = encoders.coder[4].number;

	rcp_session session;

	const AVCodec* codec_in = avcodec_find_decoder(codec_id);

	AVCodecContext* dec_ctx = avcodec_alloc_context3(codec_in);
	if (dec_ctx == NULL) {
		return -1;
		TL_ERROR("cannot allocate codec context");
	}

	dec_ctx->sample_rate = 8000;
	dec_ctx->ch_layout.nb_channels = 1;
	dec_ctx->bit_rate = 64000;
	dec_ctx->sample_fmt = AV_SAMPLE_FMT_S16P;

	if (avcodec_open2(dec_ctx, codec_in, NULL) < 0) {
		TL_ERROR("cannot open codec");
		return -1;
	}

	if (SDL_Init(SDL_INIT_AUDIO)) {
		TL_ERROR("could not initialize SDL - %s\n", SDL_GetError());
		return -1;
	}

	SDL_AudioSpec desired, obtained;

	desired.freq = dec_ctx->sample_rate;
	desired.format = AUDIO_S16SYS;
	desired.channels = 1;
	desired.silence = 0;
	desired.samples = BUF_SIZE;
	desired.callback = NULL;
	desired.userdata = NULL;

	SDL_AudioDeviceID dev = SDL_OpenAudioDevice(NULL, 0, &desired, &obtained, 0);
	if (!dev) {
		fprintf(stderr, "SDL_OpenAudioDevice failed: %s\n", SDL_GetError());
		return -1;
	}

	if (SDL_OpenAudio(&desired, &obtained) != 0) {
		TL_ERROR("error opening audio");
		return -1;
	}

	TL_INFO("%d %x %d %d %d", obtained.freq, obtained.format, obtained.channels, obtained.silence, obtained.samples);

	memset(&session, 0, sizeof(rcp_session));
	const unsigned short udp_port = stream_connect_udp(&session);

	rcp_media_descriptor desc = {
		RCP_MEP_UDP, 1, 1, 0, udp_port, 0, 1,
		RCP_AUDIO_CODING_G711_8kHz, // G.711_8kHz (sampling rate: 8 kHz, rtp clock rate: 8 kHz)
		0
	};

	if ((rc = client_connect(&session, RCP_CONNECTION_METHOD_GET,
		RCP_MEDIA_TYPE_AUDIO, 0, &desc)) < 0)
		return rc;

	pthread_create(&thread, NULL, keep_alive_thread, &session);

	SDL_PauseAudioDevice(dev, 0);

	ssize_t rtp_len;
	const int bps = av_get_bytes_per_sample(dec_ctx->sample_fmt);
	AVPacket pkt = {0};
	AVFrame frame = {0};
	unsigned char buff[BUF_SIZE];

	while (!stop) {
		if ((rtp_len = recvfrom(session.stream_socket, buff, BUF_SIZE, 0, NULL, NULL)) > 12) {
			pkt.data = buff + 12;
			pkt.size = rtp_len - 12;

			if ((rc = avcodec_send_packet(dec_ctx, &pkt)) < 0) {
				av_log(NULL, AV_LOG_ERROR, "avcodec_send_packet(): %s\n",
					   av_err2str(rc));
				break;
			}

			while (!stop && avcodec_receive_frame(dec_ctx, &frame) >= 0)
				if (SDL_QueueAudio(dev, frame.data[0], frame.nb_samples * bps) < 0) {
					av_log(NULL, AV_LOG_ERROR, "SDL_QueueAudio(): %s\n", SDL_GetError());
					stop = 1;
					break;
				}

			while (!stop && SDL_GetQueuedAudioSize(dev) > (unsigned int)(obtained.freq * bps))
				SDL_Delay(1);
		}
	}

	if (!stop) {
		while (SDL_GetQueuedAudioSize(dev) > 0) {
			SDL_Delay(10);
		}
	}

	client_disconnect(&session);
	client_unregister();
	stop_message_manager();

	return 0;
}
