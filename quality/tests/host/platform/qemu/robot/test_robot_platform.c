#include <assert.h>
#include <string.h>

#include "core/platform/qemu/robot/robot_platform.h"

static void test_platform_init_null(void)
{
    /* Should not crash or modify anything when passed NULL */
    bh_qemu_robot_platform_init(NULL);
}

static void test_platform_init_valid(void)
{
    bh_qemu_robot_platform_t platform;
    uint32_t i;

    /* Pre-fill with garbage to ensure init actually overwrites it */
    memset(&platform, 0xFF, sizeof(platform));

    bh_qemu_robot_platform_init(&platform);

    /* Check sensors initialization */
    assert(platform.sensors.timestamp_ns == 0u);
    assert(platform.sensors.sequence == 0u);

    /* Check motors initialization */
    for (i = 0; i < BH_VIRTUAL_MOTOR_COUNT; i++) {
        assert(platform.motors.motors[i].output == 0.0f);
        assert(platform.motors.motors[i].last_request_id == 0u);
        assert(platform.motors.motors[i].armed == 0u);
    }
}

int main(void)
{
    test_platform_init_null();
    test_platform_init_valid();
    return 0;
}
