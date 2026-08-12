#ifndef BHARAT_STACKS_ROBOTICS_DEMO_H
#define BHARAT_STACKS_ROBOTICS_DEMO_H

#include <stddef.h>
#include "../../platform/qemu/robot/robot_platform.h"
#include "../../services/device/actuator_mgr/actuator_mgr.h"
#include "../../services/device/sensormgr/sensormgr.h"

typedef struct bh_robotics_demo {
    bh_qemu_robot_platform_t platform;
    bh_sensormgr_t sensor_manager;
    bh_actuator_mgr_t actuator_manager;
    bh_sensor_stream_t imu_stream;
    bh_sensor_stream_t gps_stream;
    bh_sensor_stream_t battery_stream;
    bh_actuator_handle_t motors[BH_VIRTUAL_MOTOR_COUNT];
    uint64_t next_request_id;
} bh_robotics_demo_t;

bh_status_t bh_robotics_demo_init(bh_robotics_demo_t *demo);
bh_status_t bh_robotics_demo_render(bh_robotics_demo_t *demo, char *output, size_t output_size);

#endif
