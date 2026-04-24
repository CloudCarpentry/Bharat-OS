#include <stdio.h>
#include <assert.h>
#include "../../../core/drivers/display/drm/drm_core.h"
#include "../../../core/drivers/display/drm/drm_registry.h"

int mock_mode_valid(drm_device_t *dev, const drm_mode_t *mode) {
    (void)dev;
    if (mode->width == 1920 && mode->height == 1080 && mode->refresh_rate == 60) {
        return 0; // Valid
    }
    return -1; // Invalid
}

int mock_fb_create(drm_device_t *dev, uint32_t width, uint32_t height, uint32_t format, drm_fb_t **out_fb) {
    (void)dev;
    if (width == 0 || height == 0) return -1;
    // Mock allocation
    static drm_fb_t mock_fb;
    mock_fb.dev = dev;
    mock_fb.width = width;
    mock_fb.height = height;
    mock_fb.format = format;
    *out_fb = &mock_fb;
    return 0;
}

void mock_fb_destroy(drm_device_t *dev, drm_fb_t *fb) {
    (void)dev;
    (void)fb;
}

int main(void) {
    printf("Running DRM mode and fb validation tests...\n");

    extern void drm_registry_init(void);
    drm_registry_init();

    drm_device_ops_t ops = {
        .load = NULL,
        .unload = NULL,
        .mode_valid = mock_mode_valid,
        .fb_create = mock_fb_create,
        .fb_destroy = mock_fb_destroy,
        .set_scanout = NULL
    };

    drm_device_t dev = {
        .id = 0,
        .name = "mock_drm_fb",
        .ops = &ops
    };

    drm_device_register(&dev);

    // Test mode validation
    drm_mode_t valid_mode = {1920, 1080, 60, 0};
    drm_mode_t invalid_mode = {1024, 768, 60, 0};

    int ret = drm_mode_validate(&dev, &valid_mode);
    assert(ret == 0);

    ret = drm_mode_validate(&dev, &invalid_mode);
    assert(ret != 0);

    // Test nulls
    ret = drm_mode_validate(NULL, &valid_mode);
    assert(ret != 0);
    ret = drm_mode_validate(&dev, NULL);
    assert(ret != 0);

    printf("DRM mode validation working correctly.\n");

    // Test FB create
    drm_fb_t *fb = NULL;
    ret = drm_fb_create(&dev, 1920, 1080, 0, &fb);
    assert(ret == 0);
    assert(fb != NULL);
    assert(fb->width == 1920);
    assert(fb->height == 1080);
    assert(fb->dev == &dev);

    // Test invalid FB create
    fb = NULL;
    ret = drm_fb_create(&dev, 0, 1080, 0, &fb);
    assert(ret != 0);

    ret = drm_fb_create(NULL, 1920, 1080, 0, &fb);
    assert(ret != 0);

    printf("DRM FB lifecycle working correctly.\n");

    // Test destroy (mostly just checking it doesn't crash)
    drm_fb_destroy(fb);
    drm_fb_destroy(NULL);

    drm_device_unregister(&dev);
    printf("All DRM mode and FB validation tests passed!\n");

    return 0;
}