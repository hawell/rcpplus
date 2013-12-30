/*
 * audio.h
 *
 *  Created on: Oct 22, 2013
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

#ifndef AUDIO_H_
#define AUDIO_H_

#define RCP_AUDIO_INPUT_LINE	0
#define RCP_AUDIO_INPUT_MIC	1
#define RCP_AUDIO_INPUT_MUTE	2

#define RCP_COMMAND_CONF_AUDIO_INPUT_LEVEL	0x000a
#define RCP_COMMAND_CONF_AUDIO_INPUT		0x09b8
#define RCP_COMMAND_CONF_AUDIO_INPUT_MAX	0x09ba
#define RCP_COMMAND_CONF_AUDIO_MIC_LEVEL	0x09bc
#define RCP_COMMAND_CONF_AUDIO_MIC_MAX		0x09bd
#define RCP_COMMAND_CONF_AUDIO_OPTIONS		0x09bf
#define RCP_COMMAND_CONF_AUDIO_INPUT_PEEK	0x09c6

int get_audio_input_level(int line);
int set_audio_input_level(int line, int level);

int get_audio_input(int line);
int set_audio_input(int line, int input);

int get_max_audio_input_level(int line);

int get_mic_level(int line);
int set_mic_level(int line, int level);

int get_max_mic_level(int line);

int get_audio_options(int line);

int get_audio_input_peek(int line);
int set_audio_input_peek(int line, int peek);


#endif /* AUDIO_H_ */
