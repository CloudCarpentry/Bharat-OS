#include "ecu_fan.h"

#if defined(__has_include)
#  if __has_include(<bharat/uapi/syscall/bh_syscall.h>) && __has_include(<bharat/uapi/syscall_nr.h>)
#    include <bharat/uapi/syscall/bh_syscall.h>
#    include <bharat/uapi/syscall_nr.h>
#    define USE_BHARAT_SYSCALL 1
#  endif
#endif

#ifndef USE_BHARAT_SYSCALL
#  include <stdio.h>
#endif

static size_t str_len(const char *s) {
    size_t len = 0;
    while (s && s[len]) len++;
    return len;
}

static void app_log(const char *msg) {
#ifdef USE_BHARAT_SYSCALL
    (void)bharat_syscall(SYSCALL_WRITE, 1, (uintptr_t)msg, str_len(msg), 0, 0, 0);
#else
    fputs(msg, stdout);
    fflush(stdout);
#endif
}

int main(void) {
    app_log("[ECU_FAN] Application main starting...\n");

    ecu_fan_controller_t ctrl;
    ecu_fan_config_t config = {
        .temp_on_threshold = 85,
        .temp_off_threshold = 65,
        .min_valid_temp = -40,
        .max_valid_temp = 150,
        .max_consecutive_faults = 2
    };

    ecu_fan_init(&ctrl, &config, app_log);

    bool pass = ecu_fan_run_scenario_test(&ctrl);
    if (pass) {
        app_log("[ECU_FAN] All assertions passed successfully.\n");
        return 0;
    } else {
        app_log("[ECU_FAN] Error: Scenario assertions failed!\n");
        return 1;
    }
}
