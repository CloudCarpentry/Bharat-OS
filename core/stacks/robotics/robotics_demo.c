#include "robotics_demo.h"

#include <stdio.h>

static bh_status_t open_stream(bh_sensormgr_t *manager,
                               bh_sensor_type_t type,
                               bh_sensor_stream_t *stream)
{
    bh_sensor_handle_t sensor;
    bh_status_t status = bh_sensormgr_open(manager, type, &sensor);
    if (status != BH_OK) {
        return status;
    }
    return bh_sensormgr_subscribe(manager, sensor, 100u, stream);
}

bh_status_t bh_robotics_demo_init(bh_robotics_demo_t *demo)
{
    uint32_t index;
    bh_status_t status;

    if (demo == 0) {
        return BH_ERR_INVALID_ARGUMENT;
    }
    *demo = (bh_robotics_demo_t){0};
    bh_qemu_robot_platform_init(&demo->platform);
    bh_sensormgr_init(&demo->sensor_manager, &demo->platform.sensors);
    bh_actuator_mgr_init(&demo->actuator_manager, &demo->platform.motors);
    status = open_stream(&demo->sensor_manager, BH_SENSOR_IMU, &demo->imu_stream);
    if (status == BH_OK) {
        status = open_stream(&demo->sensor_manager, BH_SENSOR_GPS, &demo->gps_stream);
    }
    if (status == BH_OK) {
        status = open_stream(&demo->sensor_manager, BH_SENSOR_BATTERY, &demo->battery_stream);
    }
    if (status != BH_OK) {
        bh_actuator_mgr_enter_safe_state(&demo->actuator_manager);
        return status;
    }
    for (index = 0u; index < BH_VIRTUAL_MOTOR_COUNT; index++) {
        bh_actuator_command_t command;
        status = bh_actuator_mgr_open(&demo->actuator_manager, BH_ACTUATOR_MOTOR, index,
                                  &demo->motors[index]);
        if (status != BH_OK) {
            bh_actuator_mgr_enter_safe_state(&demo->actuator_manager);
            return status;
        }
        command = (bh_actuator_command_t){
            .abi_version = BH_ACTUATOR_ABI_VERSION,
            .flags = BH_ACTUATOR_COMMAND_ARM | BH_ACTUATOR_COMMAND_NORMALIZED,
            .request_id = ++demo->next_request_id,
            .value = 0.72f - ((float)index * 0.015f),
        };
        status = bh_actuator_mgr_write(&demo->actuator_manager, demo->motors[index], &command);
        if (status != BH_OK) {
            bh_actuator_mgr_enter_safe_state(&demo->actuator_manager);
            return status;
        }
    }
    return BH_OK;
}

bh_status_t bh_robotics_demo_render(bh_robotics_demo_t *demo, char *output, size_t output_size)
{
    bh_sensor_sample_t imu;
    bh_sensor_sample_t gps;
    bh_sensor_sample_t battery;
    bh_status_t status;
    int written;

    if (demo == 0 || output == 0 || output_size == 0u) {
        return BH_ERR_INVALID_ARGUMENT;
    }
    status = bh_sensormgr_read(&demo->sensor_manager, demo->imu_stream, &imu);
    if (status == BH_OK) {
        status = bh_sensormgr_read(&demo->sensor_manager, demo->gps_stream, &gps);
    }
    if (status == BH_OK) {
        status = bh_sensormgr_read(&demo->sensor_manager, demo->battery_stream, &battery);
    }
    if (status != BH_OK) {
        bh_actuator_mgr_enter_safe_state(&demo->actuator_manager);
        return status;
    }
    written = snprintf(output, output_size,
                       "+---- Bharat Robotics Demo ----+\n"
                       " Pitch      %5.1f deg\n Roll       %5.1f deg\n Yaw        %5.1f deg\n"
                       " Altitude   %5.0f m\n Battery    %5.0f %%\n"
                       " Motor 1    %5.0f %%\n Motor 2    %5.0f %%\n"
                       " Motor 3    %5.0f %%\n Motor 4    %5.0f %%\n"
                       " Safety state: NORMAL\n+-------------------------------+\n",
                       imu.values[0], imu.values[1], imu.values[2], gps.values[2], battery.values[0],
                       demo->platform.motors.motors[0].output * 100.0f,
                       demo->platform.motors.motors[1].output * 100.0f,
                       demo->platform.motors.motors[2].output * 100.0f,
                       demo->platform.motors.motors[3].output * 100.0f);
    if (written < 0 || (size_t)written >= output_size) {
        bh_actuator_mgr_enter_safe_state(&demo->actuator_manager);
        return BH_ERR_OVERFLOW;
    }
    return BH_OK;
}
