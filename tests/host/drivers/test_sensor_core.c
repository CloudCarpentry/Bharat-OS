#include <stdio.h>
#include <assert.h>
#include "drivers/sensor/sensor_device.h"

int dummy_read(sensor_device_t* dev, sensor_sample_t* out) { return 0; }

void test_sensor_registration() {
    sensor_device_ops_t ops = { .read_sample = dummy_read };
    sensor_device_t dev1 = { .sensor_id = 1, .ops = &ops };
    sensor_device_t dev2 = { .sensor_id = 2, .ops = &ops };

    assert(sensor_core_register(&dev1) == 0);
    assert(sensor_core_register(&dev2) == 0);

    assert(sensor_core_get(1) == &dev1);
    assert(sensor_core_get(2) == &dev2);

    // Test duplicate
    sensor_device_t dev_dup = { .sensor_id = 1, .ops = &ops };
    assert(sensor_core_register(&dev_dup) == -1);

    assert(sensor_core_unregister(&dev1) == 0);
    assert(sensor_core_get(1) == NULL);
}

int main() {
    test_sensor_registration();
    printf("test_sensor_core passed.\n");
    return 0;
}
