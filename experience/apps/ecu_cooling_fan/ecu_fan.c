#include "ecu_fan.h"

static ecu_log_fn_t g_logger = NULL;

static void internal_log(const char *msg) {
    if (g_logger) {
        g_logger(msg);
    }
}

static void log_int(int32_t val) {
    char buf[16];
    int idx = 0;
    if (val < 0) {
        buf[idx++] = '-';
        val = -val;
    }
    if (val == 0) {
        buf[idx++] = '0';
    } else {
        char temp[12];
        int t_idx = 0;
        while (val > 0) {
            temp[t_idx++] = (char)('0' + (val % 10));
            val /= 10;
        }
        while (t_idx > 0) {
            buf[idx++] = temp[--t_idx];
        }
    }
    buf[idx] = '\0';
    internal_log(buf);
}

const char *ecu_fan_state_name(ecu_fan_state_t state) {
    switch (state) {
        case ECU_STATE_NORMAL:     return "NORMAL";
        case ECU_STATE_HOT:        return "HOT";
        case ECU_STATE_COOLING:    return "COOLING";
        case ECU_STATE_SAFE_STATE: return "SAFE_STATE";
        default:                   return "UNINITIALIZED";
    }
}

void ecu_fan_init(ecu_fan_controller_t *ctrl, const ecu_fan_config_t *config, ecu_log_fn_t logger) {
    if (!ctrl) return;
    g_logger = logger;

    if (config) {
        ctrl->config = *config;
    } else {
        ctrl->config.temp_on_threshold = 85;
        ctrl->config.temp_off_threshold = 65;
        ctrl->config.min_valid_temp = -40;
        ctrl->config.max_valid_temp = 150;
        ctrl->config.max_consecutive_faults = 2;
    }

    ctrl->current_state = ECU_STATE_NORMAL;
    ctrl->fan_command = ECU_FAN_CMD_OFF;
    ctrl->last_valid_temp = 25;
    ctrl->fault_count = 0;
    ctrl->deadline_miss_count = 0;
    ctrl->total_samples_processed = 0;
    ctrl->last_sample_time_ms = 0;

    internal_log("[ECU_FAN] INIT: Cooling fan controller online\n");
    internal_log("[ECU_FAN] CONFIG: T_ON=");
    log_int(ctrl->config.temp_on_threshold);
    internal_log("C T_OFF=");
    log_int(ctrl->config.temp_off_threshold);
    internal_log("C\n");
}

bool ecu_fan_is_active(const ecu_fan_controller_t *ctrl) {
    return (ctrl && ctrl->fan_command == ECU_FAN_CMD_ON);
}

void ecu_fan_process_sample(ecu_fan_controller_t *ctrl, const ecu_sensor_sample_t *sample) {
    if (!ctrl || !sample) return;

    ctrl->total_samples_processed++;

    /* Check sample validity and bounds */
    bool valid = sample->is_valid;
    if (sample->temp_celsius < ctrl->config.min_valid_temp ||
        sample->temp_celsius > ctrl->config.max_valid_temp) {
        valid = false;
    }

    /* Check for deadline miss (expected period <= 500ms in simulation) */
    if (ctrl->last_sample_time_ms > 0 && sample->timestamp_ms > ctrl->last_sample_time_ms + 1000) {
        ctrl->deadline_miss_count++;
        internal_log("[ECU_FAN] WARNING: Sample deadline missed by ");
        log_int((int32_t)(sample->timestamp_ms - ctrl->last_sample_time_ms));
        internal_log("ms\n");
    }
    ctrl->last_sample_time_ms = sample->timestamp_ms;

    if (!valid) {
        ctrl->fault_count++;
        internal_log("[ECU_FAN] SENSOR_FAILURE: Out of bounds (TEMP=");
        log_int(sample->temp_celsius);
        internal_log("C, FAULT_COUNT=");
        log_int((int32_t)ctrl->fault_count);
        internal_log(")\n");

        /* Fail-safe action: force fan ON and transition to SAFE_STATE */
        ctrl->current_state = ECU_STATE_SAFE_STATE;
        ctrl->fan_command = ECU_FAN_CMD_ON;
        internal_log("[ECU_FAN] STATE=SAFE_STATE FAN=ON\n");
        return;
    }

    /* Valid sample received: record valid temperature */
    ctrl->last_valid_temp = sample->temp_celsius;

    /* If recovering from safe state, log recovery */
    if (ctrl->current_state == ECU_STATE_SAFE_STATE) {
        internal_log("[ECU_FAN] RECOVERY: Sensor valid (TEMP=");
        log_int(sample->temp_celsius);
        internal_log("C)\n");
        ctrl->current_state = ECU_STATE_NORMAL;
    }

    /* Hysteresis logic evaluation */
    if (sample->temp_celsius >= ctrl->config.temp_on_threshold) {
        ctrl->current_state = ECU_STATE_HOT;
        ctrl->fan_command = ECU_FAN_CMD_ON;
        internal_log("[ECU_FAN] STATE=HOT TEMP=");
        log_int(sample->temp_celsius);
        internal_log("C FAN=ON\n");
    } else if (sample->temp_celsius <= ctrl->config.temp_off_threshold) {
        ctrl->current_state = ECU_STATE_NORMAL;
        ctrl->fan_command = ECU_FAN_CMD_OFF;
        internal_log("[ECU_FAN] STATE=NORMAL TEMP=");
        log_int(sample->temp_celsius);
        internal_log("C FAN=OFF\n");
    } else {
        /* In hysteresis band: (T_OFF < temp < T_ON) */
        if (ctrl->fan_command == ECU_FAN_CMD_ON) {
            ctrl->current_state = ECU_STATE_COOLING;
            internal_log("[ECU_FAN] STATE=COOLING TEMP=");
            log_int(sample->temp_celsius);
            internal_log("C FAN=ON\n");
        } else {
            ctrl->current_state = ECU_STATE_NORMAL;
            internal_log("[ECU_FAN] STATE=NORMAL TEMP=");
            log_int(sample->temp_celsius);
            internal_log("C FAN=OFF\n");
        }
    }
}

bool ecu_fan_run_scenario_test(ecu_fan_controller_t *ctrl) {
    if (!ctrl) return false;

    internal_log("[ECU_FAN] Starting deterministic scenario sequence...\n");

    /* Step 1: Normal idle temperature (45°C) -> Expect NORMAL, Fan OFF */
    ecu_sensor_sample_t s1 = { .temp_celsius = 45, .is_valid = true, .timestamp_ms = 100 };
    ecu_fan_process_sample(ctrl, &s1);
    if (ctrl->current_state != ECU_STATE_NORMAL || ctrl->fan_command != ECU_FAN_CMD_OFF) return false;

    /* Step 2: Warm but below threshold (75°C) -> Expect NORMAL, Fan OFF */
    ecu_sensor_sample_t s2 = { .temp_celsius = 75, .is_valid = true, .timestamp_ms = 200 };
    ecu_fan_process_sample(ctrl, &s2);
    if (ctrl->current_state != ECU_STATE_NORMAL || ctrl->fan_command != ECU_FAN_CMD_OFF) return false;

    /* Step 3: Threshold breach (88°C >= 85°C) -> Expect HOT, Fan ON */
    ecu_sensor_sample_t s3 = { .temp_celsius = 88, .is_valid = true, .timestamp_ms = 300 };
    ecu_fan_process_sample(ctrl, &s3);
    if (ctrl->current_state != ECU_STATE_HOT || ctrl->fan_command != ECU_FAN_CMD_ON) return false;

    /* Step 4: Cooling down within hysteresis window (72°C) -> Expect COOLING, Fan stays ON */
    ecu_sensor_sample_t s4 = { .temp_celsius = 72, .is_valid = true, .timestamp_ms = 400 };
    ecu_fan_process_sample(ctrl, &s4);
    if (ctrl->current_state != ECU_STATE_COOLING || ctrl->fan_command != ECU_FAN_CMD_ON) return false;

    /* Step 5: Dropped below low threshold (62°C <= 65°C) -> Expect NORMAL, Fan OFF */
    ecu_sensor_sample_t s5 = { .temp_celsius = 62, .is_valid = true, .timestamp_ms = 500 };
    ecu_fan_process_sample(ctrl, &s5);
    if (ctrl->current_state != ECU_STATE_NORMAL || ctrl->fan_command != ECU_FAN_CMD_OFF) return false;

    /* Step 6: Sensor fault (-999°C, out of physical range) -> Expect SAFE_STATE, Fan forced ON */
    ecu_sensor_sample_t s6 = { .temp_celsius = -999, .is_valid = false, .timestamp_ms = 600 };
    ecu_fan_process_sample(ctrl, &s6);
    if (ctrl->current_state != ECU_STATE_SAFE_STATE || ctrl->fan_command != ECU_FAN_CMD_ON) return false;

    /* Step 7: Recovery with valid sensor reading (50°C) -> Expect NORMAL, Fan OFF */
    ecu_sensor_sample_t s7 = { .temp_celsius = 50, .is_valid = true, .timestamp_ms = 700 };
    ecu_fan_process_sample(ctrl, &s7);
    if (ctrl->current_state != ECU_STATE_NORMAL || ctrl->fan_command != ECU_FAN_CMD_OFF) return false;

    internal_log("[ECU_FAN] SCENARIO_COMPLETE: PASS\n");
    return true;
}
