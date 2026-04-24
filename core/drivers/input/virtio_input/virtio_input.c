#include "virtio_input.h"

#include <string.h>

#ifndef REL_X
#define REL_X 0x00
#endif
#ifndef REL_Y
#define REL_Y 0x01
#endif
#ifndef REL_WHEEL
#define REL_WHEEL 0x08
#endif
#ifndef BTN_LEFT
#define BTN_LEFT 0x110
#endif
#ifndef BTN_RIGHT
#define BTN_RIGHT 0x111
#endif
#ifndef BTN_MIDDLE
#define BTN_MIDDLE 0x112
#endif

#define VIRTIO_INPUT_EV_SYN 0x00
#define VIRTIO_INPUT_EV_KEY 0x01
#define VIRTIO_INPUT_EV_REL 0x02
#define VIRTIO_INPUT_EV_ABS 0x03

#define VIRTIO_INPUT_OK 0
#define VIRTIO_INPUT_EINVAL -1
#define VIRTIO_INPUT_ESTATE -2
#define VIRTIO_INPUT_ENOSPC -3

__attribute__((weak)) int bharat_input_register(bharat_input_device_t *dev) {
    (void)dev;
    return 0;
}

__attribute__((weak)) void bharat_input_report_event(bharat_input_device_t *dev, uint16_t type, uint16_t code, int32_t value) {
    (void)dev;
    (void)type;
    (void)code;
    (void)value;
}

__attribute__((weak)) void bharat_input_sync(bharat_input_device_t *dev) {
    (void)dev;
}

static bool virtio_input_supported_code(const virtio_input_device_t *dev, uint16_t type, uint16_t code) {
    if (!dev) {
        return false;
    }

    switch (type) {
        case VIRTIO_INPUT_EV_KEY:
            if (code >= BTN_LEFT) {
                return dev->supports_buttons;
            }
            return dev->supports_keys;
        case VIRTIO_INPUT_EV_REL:
            if (code == REL_WHEEL) {
                return dev->supports_wheel;
            }
            return dev->supports_rel && (code == REL_X || code == REL_Y);
        case VIRTIO_INPUT_EV_ABS:
            return dev->supports_abs;
        case VIRTIO_INPUT_EV_SYN:
            return true;
        default:
            return false;
    }
}

int virtio_input_init(virtio_input_device_t *dev,
                      virtio_input_raw_event_t *queue_buf,
                      size_t queue_depth) {
    if (!dev || !queue_buf || queue_depth == 0u) {
        return VIRTIO_INPUT_EINVAL;
    }

    memset(dev, 0, sizeof(*dev));
    dev->queue = queue_buf;
    dev->queue_depth = queue_depth;

    dev->input_dev.name = "virtio-input";
    dev->input_dev.priv_data = dev;

    return VIRTIO_INPUT_OK;
}

int virtio_input_probe(virtio_input_device_t *dev, void *device_handle) {
    if (!dev || !device_handle) {
        return VIRTIO_INPUT_EINVAL;
    }
    return VIRTIO_INPUT_OK;
}

int virtio_input_bind(virtio_input_device_t *dev, const virtio_input_caps_t *caps) {
    int rc;
    if (!dev || !caps) {
        return VIRTIO_INPUT_EINVAL;
    }

    dev->supports_keys = caps->supports_keys;
    dev->supports_rel = caps->supports_rel;
    dev->supports_buttons = caps->supports_buttons;
    dev->supports_wheel = caps->supports_wheel;
    dev->supports_abs = caps->supports_abs;

    rc = bharat_input_register(&dev->input_dev);
    if (rc != 0) {
        return rc;
    }

    return VIRTIO_INPUT_OK;
}

int virtio_input_start(virtio_input_device_t *dev) {
    if (!dev) {
        return VIRTIO_INPUT_EINVAL;
    }
    dev->started = true;
    return VIRTIO_INPUT_OK;
}

int virtio_input_stop(virtio_input_device_t *dev) {
    if (!dev) {
        return VIRTIO_INPUT_EINVAL;
    }

    dev->started = false;
    dev->irq_enabled = false;
    dev->queue_head = 0;
    dev->queue_tail = 0;
    dev->queue_used = 0;
    return VIRTIO_INPUT_OK;
}

int virtio_input_reset(virtio_input_device_t *dev) {
    int rc;
    if (!dev) {
        return VIRTIO_INPUT_EINVAL;
    }

    dev->counters.resets++;
    rc = virtio_input_stop(dev);
    if (rc != 0) {
        return rc;
    }

    return virtio_input_start(dev);
}

int virtio_input_enqueue_raw_event(virtio_input_device_t *dev,
                                   const virtio_input_raw_event_t *ev) {
    if (!dev || !ev) {
        return VIRTIO_INPUT_EINVAL;
    }

    if (dev->queue_used >= dev->queue_depth) {
        dev->counters.events_dropped++;
        return VIRTIO_INPUT_ENOSPC;
    }

    dev->queue[dev->queue_tail] = *ev;
    dev->queue_tail = (dev->queue_tail + 1u) % dev->queue_depth;
    dev->queue_used++;
    dev->counters.queue_refills++;
    return VIRTIO_INPUT_OK;
}

static int virtio_input_process_one(virtio_input_device_t *dev) {
    virtio_input_raw_event_t ev;

    if (!dev || !dev->started) {
        return VIRTIO_INPUT_ESTATE;
    }

    if (dev->queue_used == 0u) {
        return 0;
    }

    ev = dev->queue[dev->queue_head];
    dev->queue_head = (dev->queue_head + 1u) % dev->queue_depth;
    dev->queue_used--;

    if (!virtio_input_supported_code(dev, ev.type, ev.code)) {
        dev->counters.malformed_events++;
        dev->counters.events_dropped++;
        return 1;
    }

    if (ev.type == VIRTIO_INPUT_EV_REL && ev.value == 0) {
        dev->counters.events_dropped++;
        return 1;
    }

    bharat_input_report_event(&dev->input_dev, ev.type, ev.code, ev.value);
    if (ev.type != VIRTIO_INPUT_EV_SYN) {
        bharat_input_sync(&dev->input_dev);
    }
    dev->counters.events_rx++;
    return 1;
}

int virtio_input_poll(virtio_input_device_t *dev, size_t budget) {
    size_t processed = 0;
    int rc;

    if (!dev || budget == 0u) {
        return VIRTIO_INPUT_EINVAL;
    }

    while (processed < budget) {
        rc = virtio_input_process_one(dev);
        if (rc <= 0) {
            if (rc == 0) {
                break;
            }
            return rc;
        }
        processed++;
    }

    return (int)processed;
}

int virtio_input_handle_irq(virtio_input_device_t *dev, size_t budget) {
    if (!dev) {
        return VIRTIO_INPUT_EINVAL;
    }

    if (!dev->started) {
        return VIRTIO_INPUT_ESTATE;
    }

    dev->irq_enabled = true;
    dev->counters.irq_count++;
    return virtio_input_poll(dev, budget);
}

const virtio_input_counters_t *virtio_input_get_counters(const virtio_input_device_t *dev) {
    if (!dev) {
        return NULL;
    }
    return &dev->counters;
}
