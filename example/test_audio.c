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
#include <unistd.h>
#include <tlog/tlog.h>

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"

#include "rcpdefs.h"
#include "rcpplus.h"
#include "rcpcommand.h"
#include "coder.h"

#include <SDL/SDL.h>
#include <SDL/SDL_thread.h>
#include <SDL/SDL_audio.h>

int terminate = 0;

void term_handler(int param __attribute__((unused)))
{
	terminate = 1;
}

pthread_t thread;
void* keep_alive_thread(void* params)
{
	rcp_session* session = (rcp_session*)params;
	while (1)
	{
		int n = keep_alive(session);
		TL_INFO("active connections = %d", n);
		if (n < 0)
			break;

		sleep(2);
	}
	return NULL;
}

AVCodec* codec_in;
AVCodecContext *dec_ctx;

void audio_callback(void *userdata, Uint8 *stream, int len)
{
	//TL_INFO("len = %d", len);
	AVPacket pkt;
	av_init_packet(&pkt);
	rcp_session* session = (rcp_session*)userdata;
	unsigned char buff[2000];
	int size = recvfrom(session->stream_socket, buff, 1500, 0, NULL, NULL);
	struct timeval tv;
	gettimeofday(&tv, NULL);
	//TL_INFO("received : %d", tv.tv_usec);
	if (size > 0)
	{
		// skip rtp header, payload has no header
		pkt.size = size-12;
		pkt.data = buff+12;

		int len1 = len;
		int len2 = avcodec_decode_audio3(dec_ctx, (int16_t*)stream, &len1, &pkt);
		if (len2 < 0)
		{
			TL_ERROR("error decoding audio");
			return;
		}
	}

}

int main(int argc, char* argv[])
{
	tlog_init(TLOG_MODE_STDERR, TLOG_INFO, NULL);

	if (argc < 3)
	{
		TL_INFO("%s <ip> <output>\n", argv[0]);
		return 0;
	}

	rcp_connect(argv[1]);

	start_event_handler();

	client_register(RCP_USER_LEVEL_LIVE, "", RCP_REGISTRATION_TYPE_NORMAL, RCP_ENCRYPTION_MODE_MD5);

	rcp_coder_list encoders;
	get_coder_list(RCP_CODER_DECODER, RCP_MEDIA_TYPE_AUDIO, &encoders);
	for (int i=0; i<encoders.count; i++)
	{
		TL_INFO("%x %x %x %x %x", encoders.coder[i].number, encoders.coder[i].caps, encoders.coder[i].current_cap, encoders.coder[i].param_caps, encoders.coder[i].current_param);
		log_coder(TLOG_INFO, &encoders.coder[i]);
		TL_INFO("-----------------------");
	}
	int coder_id = encoders.coder[4].number;

	rcp_session session;

	avcodec_register_all();
	av_register_all();
	avfilter_register_all();

	codec_in = avcodec_find_decoder(AV_CODEC_ID_PCM_MULAW);

	dec_ctx = avcodec_alloc_context3(codec_in);
	if (dec_ctx == NULL)
	{
		TL_ERROR("cannot allocate codec context");
		return -1;
	}

	dec_ctx->sample_rate = 8000;
	dec_ctx->channels = 1;
	dec_ctx->bit_rate = 64000;
	dec_ctx->sample_fmt = AV_SAMPLE_FMT_S16P;

	if (avcodec_open2(dec_ctx, codec_in, NULL) < 0)
	{
		TL_ERROR("cannot open codec");
		return -1;
	}

	SDL_Init(SDL_INIT_EVERYTHING);

	SDL_AudioSpec desired, obtained;

	desired.freq = dec_ctx->sample_rate;
	desired.format = AUDIO_S16SYS;
	desired.channels = 1;
	desired.silence = 0;
	desired.samples = 640*2;
	desired.callback = audio_callback;
	desired.userdata = &session;

	if (SDL_OpenAudio(&desired, &obtained) != 0)
	{
		TL_ERROR("error opening audio");
		return -1;
	}
	TL_INFO("%d %x %d %d %d", obtained.freq, obtained.format, obtained.channels, obtained.silence, obtained.samples);

	memset(&session, 0, sizeof(rcp_session));
	unsigned short udp_port = stream_connect_udp(&session);

	rcp_media_descriptor desc = {
			RCP_MEP_UDP, 1, 1, 0, udp_port, 0, 1,
			4, 	// G.711_8kHz (sampling rate: 8 kHz, rtp clock rate: 8 kHz)
			0
	};

	client_connect(&session, RCP_CONNECTION_METHOD_GET, RCP_MEDIA_TYPE_AUDIO, 0, &desc);

	pthread_create(&thread, NULL, keep_alive_thread, &session);

	signal(SIGTERM, term_handler);

	SDL_PauseAudio(0);

	while (!terminate)
	{
		SDL_Event event;
		SDL_PollEvent(&event);
		if (event.type == SDL_QUIT)
		{
			SDL_Quit();
			goto end;
		}
		sleep(1);
	}

end:

	client_disconnect(&session);

	client_unregister();

	stop_event_handler();

	return 0;
}
