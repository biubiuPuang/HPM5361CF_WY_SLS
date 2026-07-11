#include <stdio.h>
#include <string.h>

#include "pedal_controller.h"

static void print_usage(const char *exe)
{
    printf("Usage: %s <COM_PORT> [--debug]\n", exe);
    printf("Example: %s COM4\n", exe);
}

static void print_axis(const char *name, const AxisStatus *axis)
{
    printf("  %s(id=%u): pos=%ld cnt rpm=%d current=%.2fA status=0x%04X running=%d fault=%d\n",
           name,
           (unsigned)axis->device_id,
           (long)axis->feedback_position_cnt,
           axis->motor_rpm,
           axis->output_current_a,
           (unsigned)axis->status_word,
           axis->is_running,
           axis->has_fault);
}

int main(int argc, char **argv)
{
    PedalController controller;
    PedalControllerConfig config;
    PedalControllerStatus status;
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

    result = pedal_controller_status(&controller, &status);
    if (result != PEDAL_OK) {
        printf("status failed: %s\n", pedal_result_to_string(result));
        pedal_controller_close(&controller);
        return 1;
    }

    printf("Controller state=%d\n", status.state);
    if (status.accelerator_valid) {
        print_axis("accelerator", &status.accelerator);
    } else {
        printf("  accelerator: invalid\n");
    }
    if (status.brake_valid) {
        print_axis("brake", &status.brake);
    } else {
        printf("  brake: invalid\n");
    }
    if (status.accelerator_valid && status.brake_valid) {
        printf("  dual_position_diff=%ld cnt warning=%d\n",
               (long)status.dual_position_diff_cnt,
               status.dual_sync_warning);
    }

    pedal_controller_close(&controller);
    return 0;
}
