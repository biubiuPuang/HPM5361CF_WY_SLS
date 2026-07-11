#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pedal_controller.h"

static void print_status(const PedalControllerStatus *status)
{
    if (!status) {
        return;
    }

    printf("state=%d last_error=%s\n", status->state, pedal_result_to_string(status->last_error));
    if (status->accelerator_valid) {
        printf("  accelerator(id=%u): pos=%ld cnt rpm=%d current=%.2fA status=0x%04X fault=%d\n",
               (unsigned)status->accelerator.device_id,
               (long)status->accelerator.feedback_position_cnt,
               status->accelerator.motor_rpm,
               status->accelerator.output_current_a,
               (unsigned)status->accelerator.status_word,
               status->accelerator.has_fault);
    } else {
        printf("  accelerator: invalid\n");
    }

    if (status->brake_valid) {
        printf("  brake(id=%u):       pos=%ld cnt rpm=%d current=%.2fA status=0x%04X fault=%d\n",
               (unsigned)status->brake.device_id,
               (long)status->brake.feedback_position_cnt,
               status->brake.motor_rpm,
               status->brake.output_current_a,
               (unsigned)status->brake.status_word,
               status->brake.has_fault);
    } else {
        printf("  brake: invalid\n");
    }

    if (status->accelerator_valid && status->brake_valid) {
        printf("  dual_position_diff=%ld cnt warning=%d\n",
               (long)status->dual_position_diff_cnt,
               status->dual_sync_warning);
    }
}

static void print_usage(const char *exe)
{
    printf("Usage: %s <COM_PORT> <SAFE_RETRACT_CNT> [--debug]\n", exe);
    printf("Example:\n");
    printf("  %s COM4 0\n", exe);
    printf("  %s COM4 -500\n", exe);
    printf("\n");
    printf("WARNING: This program sends real position commands to the dual pedals.\n");
    printf("Ensure the original host has been disconnected by your switching layer,\n");
    printf("the coordinate system is trusted, and hardware emergency stop is ready.\n");
}

int main(int argc, char **argv)
{
    PedalController controller;
    PedalControllerConfig config;
    PedalControllerStatus status;
    PedalResult result;
    char *endptr = NULL;
    long target;

    if (argc < 3) {
        print_usage(argv[0]);
        return 1;
    }

    target = strtol(argv[2], &endptr, 10);
    if (!endptr || *endptr != '\0') {
        printf("Invalid SAFE_RETRACT_CNT: %s\n", argv[2]);
        return 1;
    }

    config = pedal_controller_default_config(argv[1]);
    config.safe_retract_cnt = (int32_t)target;
    if (argc >= 4 && strcmp(argv[3], "--debug") == 0) {
        config.debug = 1;
    }

    printf("Opening dual pedal controller on %s, safe_retract_cnt=%ld cnt\n", argv[1], target);
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

    printf("Initial status:\n");
    result = pedal_controller_status(&controller, &status);
    if (result == PEDAL_OK) {
        print_status(&status);
    } else {
        printf("status read failed: %s\n", pedal_result_to_string(result));
    }

    printf("Moving both pedals to safe retract position...\n");
    result = pedal_controller_retract_to_safe(&controller, 1);
    if (result != PEDAL_OK) {
        printf("retract failed: %s\n", pedal_result_to_string(result));
        (void)pedal_controller_stop_all(&controller);
        pedal_controller_close(&controller);
        return 1;
    }

    printf("Final status:\n");
    result = pedal_controller_status(&controller, &status);
    if (result == PEDAL_OK) {
        print_status(&status);
    } else {
        printf("status read failed: %s\n", pedal_result_to_string(result));
    }

    pedal_controller_close(&controller);
    printf("Done.\n");
    return 0;
}
