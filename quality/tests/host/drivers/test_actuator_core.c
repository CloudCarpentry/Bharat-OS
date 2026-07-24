#include <stdio.h>
#include <assert.h>
#include "drivers/actuator/actuator_device.h"

int dummy_arm(actuator_device_t* dev) { return 0; }
int dummy_disarm(actuator_device_t* dev) { return 0; }

void test_actuator_registration() {
    actuator_device_ops_t ops = { .arm = dummy_arm, .disarm = dummy_disarm };
    actuator_device_t dev1 = { .actuator_id = 1, .ops = &ops };
    actuator_device_t dev2 = { .actuator_id = 2, .ops = &ops };

    assert(actuator_core_register(&dev1) == 0);
    assert(actuator_core_register(&dev2) == 0);

    assert(actuator_core_get(1) == &dev1);
    assert(actuator_core_get(2) == &dev2);

    // Duplicate test
    actuator_device_t dev_dup = { .actuator_id = 1, .ops = &ops };
    assert(actuator_core_register(&dev_dup) == -1);

    assert(actuator_core_unregister(&dev1) == 0);
    assert(actuator_core_get(1) == NULL);
}

int main() {
    test_actuator_registration();
    printf("test_actuator_core passed.\n");
    return 0;
}
