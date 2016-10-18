/*
 * bulk_config.c
 *
 *  Created on: Apr 20, 2016
 *      Author: arash
 */

#include <getopt.h>

#include <tlog/tlog.h>

#include "device.c"
#include "rcpdefs.h"
#include "rcpplus.h"
#include "rcpcommand.h"
#include "rtp.h"
#include "coder.h"
#include "preset.h"
#include "caps.h"
#include "alarm.h"

void alarm_handler(rcp_packet* data, void* param)
{
    TL_INFO("alarm");
    UNUSED(param);
    tlog_hex(TLOG_INFO, "payload : ", data->payload, data->payload_length);
}

void update_preset(char* ip)
{
    rcp_connect(ip);
    start_message_manager();
    client_register(RCP_USER_LEVEL_SERVICE, "", RCP_REGISTRATION_TYPE_NORMAL, RCP_ENCRYPTION_MODE_PLAIN);
    TL_INFO("------------------------");

    int mode[] = {3,3};
    set_h264_encoder_video_operation_mode(mode);

    for (int i=1; i<=8; i++)
    {
        rcp_mpeg4_preset preset;
        get_preset(i, &preset, 0);
        preset.bandwidth = 800;
        preset.bandwidth_soft_limit = 1200;
        preset.bandwidth_hard_limit = 0;
        preset.frameskip_ratio = 2;
        preset.resolution = 10;
        preset.averaging_period = 0;
        preset.avc_gop_structure = 0;
        preset.iframe_distance = 0;
        preset.avc_pframe_qunatizer_min = 0;
        preset.avc_delta_ipquantizer = 5;
        preset.avc_quant_adj_background = 13;
        preset.avc_quant_adj_objects = -12;
        set_preset(i, &preset, 0);
    }
    TL_INFO("------------------------");

    client_unregister();
    stop_message_manager();
    rcp_disconnect();
    TL_INFO("------------------------");

}

void set_camera_defaults(char* ip)
{
    rcp_connect(ip);
    start_message_manager();
    client_register(RCP_USER_LEVEL_SERVICE, "", RCP_REGISTRATION_TYPE_NORMAL, RCP_ENCRYPTION_MODE_PLAIN);

    set_defaults();

    client_unregister();
    stop_message_manager();
    rcp_disconnect();
}

void reset_camera(char* ip)
{
    rcp_connect(ip);
    start_message_manager();
    client_register(RCP_USER_LEVEL_SERVICE, "", RCP_REGISTRATION_TYPE_NORMAL, RCP_ENCRYPTION_MODE_PLAIN);

    board_reset();

    client_unregister();
    stop_message_manager();
    rcp_disconnect();
}

enum Function {
    Default,
    Reset,
    Config
};

int main(int argc, char* argv[])
{
    rcp_device *devs;
    int num;

    int has_ip = 0;
    char ip[20];

    tlog_init(TLOG_MODE_STDERR, TLOG_DEBUG, NULL);

    static struct option long_options[] = {
                    {"default", 0, 0, 'd'},
                    {"config", 0, 0, 'c'},
                    {"reset", 0, 0, 'r'},
                    {"address", 1, 0, 'a'}
    };

    enum Function f;
    while (1)
    {
        int option_index = 0;

        int c = getopt_long (argc, argv, "dcra:", long_options, &option_index);
        if (c == -1)
        {
            break;
        }

        switch (c)
        {
            case 'd':
                f = Default;
                break;
            case 'c':
                f = Config;
                break;
            case 'r':
                f = Reset;
                break;
            case 'a':
                has_ip = 1;
                strcpy(ip, optarg);
                break;
        }
    }


    if (has_ip)
    {
        switch (f)
        {
            case Default:
                set_camera_defaults(ip);
                break;
            case Config:
                update_preset(ip);
                break;
            case Reset:
                reset_camera(ip);
                break;
        }
    }
    else
    {
        autodetect(&devs, &num);

        TL_INFO("%d device%s detected", num, num>1?"s":"");
        for (int i=0; i<num; i++)
        {
            log_device(TLOG_INFO, &devs[i]);
            TL_INFO("------------------------");

            sprintf(ip, "%d.%d.%d.%d", devs[i].address[0], devs[i].address[1], devs[i].address[2], devs[i].address[3]);
            switch (f)
            {
                case Default:
                    set_camera_defaults(ip);
                    break;
                case Config:
                    update_preset(ip);
                    break;
                case Reset:
                    reset_camera(ip);
                    break;
            }
        }
        free(devs);
    }


    return 0;
}
