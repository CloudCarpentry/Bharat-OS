#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "../../../drivers/input/virtio_input/virtio_input.h"
#include "../../../interface/uapi/bharat/input/input.h"

#ifndef REL_X
#define REL_X 0x00
#endif
#ifndef BTN_LEFT
#define BTN_LEFT 0x110
#endif

static int g_report_count = 0;
static int g_sync_count = 0;
static uint16_t g_last_type = 0;
static uint16_t g_last_code = 0;
static int32_t g_last_value = 0;

int bharat_input_register(bharat_input_device_t *dev) {
    (void)dev;
    return 0;
}

void bharat_input_report_event(bharat_input_device_t *dev, uint16_t type, uint16_t code, int32_t value) {
    (void)dev;
    g_report_count++;
    g_last_type = type;
    g_last_code = code;
    g_last_value = value;
}

void bharat_input_sync(bharat_input_device_t *dev) {
    (void)dev;
    g_sync_count++;
}

static void reset_capture(void) {
    g_report_count = 0;
    g_sync_count = 0;
    g_last_type = 0;
    g_last_code = 0;
    g_last_value = 0;
}

int main(void) {
    virtio_input_device_t dev;
    virtio_input_raw_event_t queue[2];
    virtio_input_caps_t caps = {
        .supports_keys = true,
        .supports_rel = true,
        .supports_buttons = true,
        .supports_wheel = true,
        .supports_abs = false,
    };

    assert(virtio_input_init(&dev, queue, 2) == 0);
    assert(virtio_input_probe(&dev, &dev) == 0);
    assert(virtio_input_bind(&dev, &caps) == 0);
    assert(virtio_input_start(&dev) == 0);

    /* feature negotiation + event decode */
    assert(virtio_input_enqueue_raw_event(&(dev), &(virtio_input_raw_event_t){.type = EV_KEY, .code = KEY_A, .value = 1}) == 0);
    assert(virtio_input_poll(&dev, 4) == 1);
    assert(g_report_count == 1);
    assert(g_sync_count == 1);
    assert(g_last_type == EV_KEY);
    assert(g_last_code == KEY_A);

    reset_capture();
    assert(virtio_input_enqueue_raw_event(&(dev), &(virtio_input_raw_event_t){.type = EV_REL, .code = REL_X, .value = 5}) == 0);
    assert(virtio_input_handle_irq(&dev, 4) == 1);
    assert(g_report_count == 1);
    assert(g_sync_count == 1);
    assert(virtio_input_get_counters(&dev)->irq_count == 1);

    /* malformed drop */
    assert(virtio_input_enqueue_raw_event(&(dev), &(virtio_input_raw_event_t){.type = 0x99, .code = 0, .value = 0}) == 0);
    assert(virtio_input_poll(&dev, 1) == 1);
    assert(virtio_input_get_counters(&dev)->malformed_events == 1);

    /* queue exhaustion */
    assert(virtio_input_enqueue_raw_event(&(dev), &(virtio_input_raw_event_t){.type = EV_KEY, .code = KEY_B, .value = 1}) == 0);
    assert(virtio_input_enqueue_raw_event(&(dev), &(virtio_input_raw_event_t){.type = EV_KEY, .code = KEY_C, .value = 1}) == 0);
    assert(virtio_input_enqueue_raw_event(&(dev), &(virtio_input_raw_event_t){.type = EV_KEY, .code = KEY_D, .value = 1}) != 0);

    /* stop/reset path */
    assert(virtio_input_stop(&dev) == 0);
    assert(virtio_input_poll(&dev, 1) < 0);
    assert(virtio_input_reset(&dev) == 0);
    assert(dev.started);
    assert(virtio_input_get_counters(&dev)->resets == 1);

    puts("virtio_input host tests passed");
    return 0;
}
