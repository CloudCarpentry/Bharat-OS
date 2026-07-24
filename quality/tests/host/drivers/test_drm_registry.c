#include <stdio.h>
#include <assert.h>
#include "../../../core/drivers/display/drm/drm_registry.h"

int mock_load(drm_device_t *dev) {
    (void)dev;
    return 0;
}

void mock_unload(drm_device_t *dev) {
    (void)dev;
}

int main(void) {
    printf("Running DRM registry tests...\n");

    extern void drm_registry_init(void);
    drm_registry_init();

    drm_device_ops_t ops = {
        .load = mock_load,
        .unload = mock_unload,
        .mode_valid = NULL,
        .fb_create = NULL,
        .fb_destroy = NULL,
        .set_scanout = NULL
    };

    drm_device_t dev = {
        .id = 0,
        .name = "mock_drm",
        .ops = &ops
    };

    // Test successful registration
    int ret = drm_device_register(&dev);
    assert(ret == 0);
    printf("Registered valid DRM device successfully.\n");

    // Test duplicate registration
    ret = drm_device_register(&dev);
    assert(ret != 0); // Should fail
    printf("Duplicate DRM registration handled correctly.\n");

    // Test get device
    drm_device_t *fetched = drm_get_device(0);
    assert(fetched == &dev);
    printf("Fetched DRM device correctly.\n");

    // Test invalid device ID
    fetched = drm_get_device(-1);
    assert(fetched == NULL);
    fetched = drm_get_device(100);
    assert(fetched == NULL);
    printf("Handled invalid DRM device IDs correctly.\n");

    // Test unregister
    drm_device_unregister(&dev);
    fetched = drm_get_device(0);
    assert(fetched == NULL);
    printf("Unregistered DRM device successfully.\n");

    // Test invalid device registration
    drm_device_t invalid_dev_1 = {
        .id = 1,
        .ops = NULL // Missing ops
    };
    ret = drm_device_register(&invalid_dev_1);
    assert(ret != 0);
    printf("Invalid DRM device registrations rejected correctly.\n");

    printf("All DRM registry tests passed!\n");
    return 0;
}