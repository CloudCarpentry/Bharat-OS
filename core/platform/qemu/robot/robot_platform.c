#include "robot_platform.h"

void bh_qemu_robot_platform_init(bh_qemu_robot_platform_t *platform)
{
    if (platform == 0) {
        return;
    }
    bh_virtual_sensors_init(&platform->sensors);
    bh_virtual_motors_init(&platform->motors);
}
