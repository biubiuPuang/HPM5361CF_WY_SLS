#include <stdio.h>
#include <string.h>

#include "pedal_controller.h"

static void print_usage(const char *exe)
{
    printf("Usage: %s <COM_PORT> [--debug]\n", exe);
    printf("Example: %s COM4\n", exe);
    printf("WARNING: software emergency stop cannot replace hardware emergency stop.\n");
}

int main(int argc, char **argv)
{
    PedalController controller;
    PedalControllerConfig config;
    PedalResult result;

    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    config = pedal_controller_default_config(argv[1]);
    if (argc >= 3 && strcmp(argv[2], "--debug") == 0) {
        config.debug = 1;
    }

    result = pedal_controller_init(&controller, &config);
    if (result != PEDAL_OK) {
        printf("init failed: %s\n", pedal_result_to_string(result));
        return 1;
    }

    result = pedal_controller_open(&controller);
    if (result != PEDAL_OK) {
        printf("open failed: %s\n", pedal_result_to_string(result));
        return 1;
    }

    result = pedal_controller_emergency_stop_all(&controller);
    if (result != PEDAL_OK) {
        printf("emergency stop failed: %s\n", pedal_result_to_string(result));
        pedal_controller_close(&controller);
        return 1;
    }

    printf("Software emergency stop command sent to both pedals.\n");
    pedal_controller_close(&controller);
    return 0;
}
