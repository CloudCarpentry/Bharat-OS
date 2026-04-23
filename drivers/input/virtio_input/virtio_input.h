#ifndef BHARAT_VIRTIO_INPUT_H
#define BHARAT_VIRTIO_INPUT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <bharat/input/input_driver.h>

typedef struct {
    uint16_t type;
    uint16_t code;
    int32_t value;
} virtio_input_raw_event_t;

typedef struct {
    uint64_t events_rx;
    uint64_t events_dropped;
    uint64_t malformed_events;
    uint64_t queue_refills;
    uint64_t irq_count;
    uint64_t resets;
} virtio_input_counters_t;

typedef struct {
    bool supports_keys;
    bool supports_rel;
    bool supports_buttons;
    bool supports_wheel;
    bool supports_abs;
} virtio_input_caps_t;

typedef struct {
    bool started;
    bool irq_enabled;
    bool supports_keys;
    bool supports_rel;
    bool supports_buttons;
    bool supports_wheel;
    bool supports_abs;

    size_t queue_depth;
    size_t queue_head;
    size_t queue_tail;
    size_t queue_used;
    virtio_input_raw_event_t *queue;

    bharat_input_device_t input_dev;
    virtio_input_counters_t counters;
} virtio_input_device_t;

int virtio_input_init(virtio_input_device_t *dev,
                      virtio_input_raw_event_t *queue_buf,
                      size_t queue_depth);
int virtio_input_probe(virtio_input_device_t *dev, void *device_handle);
int virtio_input_bind(virtio_input_device_t *dev, const virtio_input_caps_t *caps);
int virtio_input_start(virtio_input_device_t *dev);
int virtio_input_stop(virtio_input_device_t *dev);
int virtio_input_reset(virtio_input_device_t *dev);

int virtio_input_enqueue_raw_event(virtio_input_device_t *dev,
                                   const virtio_input_raw_event_t *ev);
int virtio_input_poll(virtio_input_device_t *dev, size_t budget);
int virtio_input_handle_irq(virtio_input_device_t *dev, size_t budget);

const virtio_input_counters_t *virtio_input_get_counters(const virtio_input_device_t *dev);

#endif
