#include <assert.h>
#include <string.h>

#include "core/stacks/robotics/robotics_demo.h"

static void test_demo_data_path(void)
{
    bh_robotics_demo_t demo;
    char output[768];
    uint32_t index;

    assert(bh_robotics_demo_init(&demo) == BH_OK);
    assert(bh_robotics_demo_render(&demo, output, sizeof(output)) == BH_OK);
    assert(strstr(output, "Bharat Robotics Demo") != 0);
    assert(strstr(output, "Safety state: NORMAL") != 0);
    for (index = 0u; index < BH_VIRTUAL_MOTOR_COUNT; index++) {
        assert(demo.platform.motors.motors[index].armed == 1u);
        assert(demo.platform.motors.motors[index].output > 0.0f);
    }
}

static void test_stale_stream_and_actuator_replay_fail_closed(void)
{
    bh_robotics_demo_t demo;
    bh_sensor_sample_t sample;
    bh_actuator_command_t replay = {
        .abi_version = BH_ACTUATOR_ABI_VERSION,
        .flags = BH_ACTUATOR_COMMAND_NORMALIZED,
        .request_id = 1u,
        .value = 1.0f,
    };
    bh_sensor_stream_t stale;

    assert(bh_robotics_demo_init(&demo) == BH_OK);
    stale = demo.imu_stream;
    assert(bh_sensormgr_unsubscribe(&demo.sensor_manager, stale) == BH_OK);
    assert(bh_sensormgr_read(&demo.sensor_manager, stale, &sample) == BH_ERR_STALE_CAPABILITY);
    assert(bh_actuator_mgr_write(&demo.actuator_manager, demo.motors[0], &replay) ==
           BH_ERR_STALE_CAPABILITY);
    assert(demo.platform.motors.motors[0].output > 0.0f);
    bh_actuator_mgr_enter_safe_state(&demo.actuator_manager);
    assert(demo.platform.motors.motors[0].output == 0.0f);
    assert(demo.platform.motors.motors[0].armed == 0u);
}

int main(void)
{
    test_demo_data_path();
    test_stale_stream_and_actuator_replay_fail_closed();
    return 0;
}
