/*
 * utils.c
 *
 *  Created on: May 7, 2016
 *      Author: arash
 */

#include "bc_debug.h"

#include "coder.h"
#include "preset.h"
#include "rcpcommand.h"


int get_resolution(int line, int stream, int* width, int* height, const char** name)
{
    int preset_id = get_coder_preset(stream);
    int mode = -1;
    get_coder_video_operation_mode(stream, &mode);
    Info("mode = %d coder = %d", mode, stream);

    rcp_mpeg4_preset preset;
    get_preset(preset_id, &preset, 1);

    int input_format = RCP_VIDEO_INPUT_FORMAT_NONE;
    int input_frequency;
    if (get_video_input_frequency(line, &input_frequency) == 0)
    {
        if (input_frequency == 30000 || input_frequency == 60000)
            input_format = RCP_VIDEO_INPUT_FORMAT_NTSC;
        else if (input_frequency == 25000 || input_frequency == 50000)
            input_format = RCP_VIDEO_INPUT_FORMAT_PAL;
    }
    if (input_format == RCP_VIDEO_INPUT_FORMAT_NONE)
        get_video_input_format(line, &input_format);

    int modes[2];
    if (mode == RCP_CODER_MODE_H264 && get_h264_encoder_video_operation_mode(modes) == 0)
    {
        if (get_resolution_from_h264_operation_mode(modes[stream-1], width, height, name) != 0)
        {
            get_resolution_from_preset(&preset, input_format, width, height, name);
        }
    }
    else
    {
        get_resolution_from_preset(&preset, input_format, width, height, name);
    }

    return 0;
}
