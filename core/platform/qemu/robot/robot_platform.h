#ifndef BHARAT_PLATFORM_QEMU_ROBOT_H
#define BHARAT_PLATFORM_QEMU_ROBOT_H

#include "../../../drivers/actuator/virtual/virtual_motors.h"
#include "../../../drivers/sensor/virtual/virtual_sensors.h"

/* QEMU robot hardware is instance-owned and contains no cross-core mutable state. */
typedef struct bh_qemu_robot_platform {
    bh_virtual_sensor_bank_t sensors;
    bh_virtual_motor_bank_t motors;
} bh_qemu_robot_platform_t;

void bh_qemu_robot_platform_init(bh_qemu_robot_platform_t *platform);

#endif
