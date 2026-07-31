#include "virtio_input.h"

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

#define VIRTIO_INPUT_CFG_ID_NAME  0x01
#define VIRTIO_INPUT_CFG_EV_BITS  0x11

#define VIRTIO_INPUT_OK 0
#define VIRTIO_INPUT_EINVAL -1
#define VIRTIO_INPUT_ESTATE -2
#define VIRTIO_INPUT_ENOSPC -3

struct virtio_input_config {
    uint8_t select;
    uint8_t subsel;
    uint8_t size;
    uint8_t reserved[5];
    union {
        char string[128];
        uint8_t bitmap[128];
    } u;
};

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

    __builtin_memset(dev, 0, sizeof(*dev));
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

    pci_device_t *pci = (pci_device_t *)device_handle;
    if (pci && pci->vendor_id == 0x1AF4 && (pci->device_id == 0x1052 || pci->device_id == 0x1012)) {
        // Probe real modern PCI device
        int rc = bh_virtio_pci_probe(&dev->vpci, pci);
        if (rc != 0) {
            return rc;
        }

        dev->is_real_pci = true;

        // Query capabilities from config space
        volatile struct virtio_input_config *cfg = (volatile struct virtio_input_config *)dev->vpci.device_cfg;
        if (cfg) {
            // Check EV_REL (0x02) supports relative axis
            cfg->select = VIRTIO_INPUT_CFG_EV_BITS;
            cfg->subsel = VIRTIO_INPUT_EV_REL;
            __asm__ volatile("" ::: "memory");
            if (cfg->size > 0) {
                dev->supports_rel = true;
                dev->supports_buttons = true;
                dev->supports_wheel = true;
                dev->input_dev.name = "VirtIO Mouse Device";
            } else {
                // Otherwise it's a keyboard
                dev->supports_keys = true;
                dev->input_dev.name = "VirtIO Keyboard Device";
            }
        }

        // Register device
        dev->input_dev.priv_data = dev;
        rc = bharat_input_register(&dev->input_dev);
        if (rc != 0) {
            return rc;
        }

        return VIRTIO_INPUT_OK;
    }

    return VIRTIO_INPUT_OK;
}

int virtio_input_bind(virtio_input_device_t *dev, const virtio_input_caps_t *caps) {
    int rc;
    if (!dev || !caps) {
        return VIRTIO_INPUT_EINVAL;
    }

    if (dev->is_real_pci) {
        return VIRTIO_INPUT_OK; // Real device binding done in probe
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

    if (dev->is_real_pci) {
        // Setup Event Queue (Queue 0)
        int rc = bh_virtio_pci_setup_queue(&dev->vpci, 0, &dev->real_vq, dev->desc_table, dev->avail_ring, dev->used_ring);
        if (rc != 0) {
            return rc;
        }

        // Post all rx event buffers
        for (int i = 0; i < INPUT_QUEUE_SIZE; i++) {
            uint16_t desc_idx = 0;
            bh_virtqueue_add_rx_buffer(&dev->real_vq, &dev->buffers[i], sizeof(virtio_input_raw_event_t), &desc_idx);
            dev->desc_to_buf[desc_idx] = (uint16_t)i;
        }

        // Notify device of posted buffers
        bh_virtio_pci_notify_queue(&dev->vpci, 0, &dev->real_vq);

        // Start device status lifecycle
        rc = bh_virtio_pci_start_device(&dev->vpci);
        if (rc != 0) {
            return rc;
        }
    }

    dev->started = true;
    return VIRTIO_INPUT_OK;
}

int virtio_input_stop(virtio_input_device_t *dev) {
    if (!dev) {
        return VIRTIO_INPUT_EINVAL;
    }

    if (dev->is_real_pci) {
        bh_virtio_pci_reset_device(&dev->vpci);
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
    if (!dev || budget == 0u) {
        return VIRTIO_INPUT_EINVAL;
    }

    if (dev->is_real_pci) {
        size_t processed = 0;
        uint16_t desc_idx;
        uint32_t len;
        while (processed < budget && bh_virtqueue_poll_used(&dev->real_vq, &desc_idx, &len)) {
            uint16_t buf_idx = dev->desc_to_buf[desc_idx];
            virtio_input_raw_event_t *ev = &dev->buffers[buf_idx];

            if (virtio_input_supported_code(dev, ev->type, ev->code)) {
                bharat_input_report_event(&dev->input_dev, ev->type, ev->code, ev->value);
                if (ev->type != VIRTIO_INPUT_EV_SYN) {
                    bharat_input_sync(&dev->input_dev);
                }
                dev->counters.events_rx++;
            } else {
                dev->counters.malformed_events++;
                dev->counters.events_dropped++;
            }

            // Repost descriptor and buffer
            bh_virtqueue_free_descriptor(&dev->real_vq, desc_idx);
            bh_virtqueue_add_rx_buffer(&dev->real_vq, &dev->buffers[buf_idx], sizeof(virtio_input_raw_event_t), &desc_idx);
            dev->desc_to_buf[desc_idx] = buf_idx;

            processed++;
        }
        if (processed > 0) {
            bh_virtio_pci_notify_queue(&dev->vpci, 0, &dev->real_vq);
        }
        return (int)processed;
    }

    // Software queue fallback
    size_t processed = 0;
    int rc;
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
