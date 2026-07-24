#include <stdio.h>
#include <assert.h>
#include "drivers/can/can_controller.h"

void test_controller_registration() {
    can_controller_ops_t ops = {0};
    can_controller_t ctrl = {
        .name = "can-test",
        .controller_id = 0,
        .ops = &ops,
        .caps = {
            .classical_can = true,
            .min_bitrate = 10000,
            .max_bitrate = 1000000,
            .min_data_bitrate = 10000,
            .max_data_bitrate = 1000000,
        }
    };

    assert(can_controller_core_register(&ctrl) == 0);
    assert(can_controller_core_get(0) == &ctrl);

    // Duplicate registration should fail
    assert(can_controller_core_register(&ctrl) == -1);

    assert(can_controller_core_unregister(&ctrl) == 0);
    assert(can_controller_core_get(0) == NULL);
}

int main() {
    test_controller_registration();
    printf("test_can_controller_core passed.\n");
    return 0;
}
