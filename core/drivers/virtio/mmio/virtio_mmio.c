#include "virtio_mmio.h"

#include <stddef.h>

#define BH_VIRTIO_MMIO_MAGIC_VALUE UINT32_C(0x000)
#define BH_VIRTIO_MMIO_VERSION UINT32_C(0x004)
#define BH_VIRTIO_MMIO_DEVICE_ID UINT32_C(0x008)
#define BH_VIRTIO_MMIO_VENDOR_ID UINT32_C(0x00c)
#define BH_VIRTIO_MMIO_DEVICE_FEATURES UINT32_C(0x010)
#define BH_VIRTIO_MMIO_DEVICE_FEATURES_SEL UINT32_C(0x014)
#define BH_VIRTIO_MMIO_DRIVER_FEATURES UINT32_C(0x020)
#define BH_VIRTIO_MMIO_DRIVER_FEATURES_SEL UINT32_C(0x024)
#define BH_VIRTIO_MMIO_QUEUE_SEL UINT32_C(0x030)
#define BH_VIRTIO_MMIO_QUEUE_NUM_MAX UINT32_C(0x034)
#define BH_VIRTIO_MMIO_QUEUE_NUM UINT32_C(0x038)
#define BH_VIRTIO_MMIO_QUEUE_READY UINT32_C(0x044)
#define BH_VIRTIO_MMIO_QUEUE_NOTIFY UINT32_C(0x050)
#define BH_VIRTIO_MMIO_STATUS UINT32_C(0x070)
#define BH_VIRTIO_MMIO_QUEUE_DESC_LOW UINT32_C(0x080)
#define BH_VIRTIO_MMIO_QUEUE_DESC_HIGH UINT32_C(0x084)
#define BH_VIRTIO_MMIO_QUEUE_AVAIL_LOW UINT32_C(0x090)
#define BH_VIRTIO_MMIO_QUEUE_AVAIL_HIGH UINT32_C(0x094)
#define BH_VIRTIO_MMIO_QUEUE_USED_LOW UINT32_C(0x0a0)
#define BH_VIRTIO_MMIO_QUEUE_USED_HIGH UINT32_C(0x0a4)

static uint32_t mmio_read32(const bh_virtio_mmio_device_t *device,
                            uint32_t offset) {
  return *(volatile const uint32_t *)(device->registers + offset);
}

static void mmio_write32(const bh_virtio_mmio_device_t *device, uint32_t offset,
                         uint32_t value) {
  *(volatile uint32_t *)(device->registers + offset) = value;
}

static void mmio_write64_pair(const bh_virtio_mmio_device_t *device,
                              uint32_t low_offset, uint32_t high_offset,
                              uint64_t value) {
  mmio_write32(device, low_offset, (uint32_t)value);
  mmio_write32(device, high_offset, (uint32_t)(value >> 32));
}

static bool is_power_of_two(uint16_t value) {
  return value != 0U && (value & (uint16_t)(value - 1U)) == 0U;
}

static void set_failed(bh_virtio_mmio_device_t *device) {
  uint32_t status = mmio_read32(device, BH_VIRTIO_MMIO_STATUS);
  mmio_write32(device, BH_VIRTIO_MMIO_STATUS, status | BH_VIRTIO_STATUS_FAILED);
  device->features_accepted = false;
}

bh_virtio_mmio_result_t bh_virtio_mmio_probe(bh_virtio_mmio_device_t *device,
                                             volatile void *registers) {
  if (device == NULL || registers == NULL) {
    return BH_VIRTIO_MMIO_INVALID_ARGUMENT;
  }

  __builtin_memset(device, 0, sizeof(*device));
  device->registers = (volatile uint8_t *)registers;
  if (mmio_read32(device, BH_VIRTIO_MMIO_MAGIC_VALUE) != BH_VIRTIO_MMIO_MAGIC) {
    device->registers = NULL;
    return BH_VIRTIO_MMIO_NOT_DEVICE;
  }
  if (mmio_read32(device, BH_VIRTIO_MMIO_VERSION) !=
      BH_VIRTIO_MMIO_VERSION_MODERN) {
    device->registers = NULL;
    return BH_VIRTIO_MMIO_UNSUPPORTED;
  }

  device->device_id = mmio_read32(device, BH_VIRTIO_MMIO_DEVICE_ID);
  device->vendor_id = mmio_read32(device, BH_VIRTIO_MMIO_VENDOR_ID);
  if (device->device_id == 0U) {
    device->registers = NULL;
    return BH_VIRTIO_MMIO_NOT_DEVICE;
  }

  mmio_write32(device, BH_VIRTIO_MMIO_STATUS, 0U);
  mmio_write32(device, BH_VIRTIO_MMIO_STATUS,
               BH_VIRTIO_STATUS_ACKNOWLEDGE | BH_VIRTIO_STATUS_DRIVER);
  return BH_VIRTIO_MMIO_OK;
}

bh_virtio_mmio_result_t bh_virtio_mmio_negotiate_features(
    bh_virtio_mmio_device_t *device, uint64_t supported_features,
    uint64_t required_features, uint64_t *negotiated_features) {
  uint64_t available;
  uint64_t agreed;
  uint32_t status;

  if (device == NULL || device->registers == NULL ||
      negotiated_features == NULL) {
    return BH_VIRTIO_MMIO_INVALID_ARGUMENT;
  }

  mmio_write32(device, BH_VIRTIO_MMIO_DEVICE_FEATURES_SEL, 0U);
  available = mmio_read32(device, BH_VIRTIO_MMIO_DEVICE_FEATURES);
  mmio_write32(device, BH_VIRTIO_MMIO_DEVICE_FEATURES_SEL, 1U);
  available |= ((uint64_t)mmio_read32(device, BH_VIRTIO_MMIO_DEVICE_FEATURES))
               << 32;
  device->device_features = available;
  agreed = available & supported_features;
  if ((agreed & required_features) != required_features ||
      (agreed & BH_VIRTIO_F_VERSION_1) == 0U) {
    set_failed(device);
    return BH_VIRTIO_MMIO_FEATURE_REJECTED;
  }

  mmio_write32(device, BH_VIRTIO_MMIO_DRIVER_FEATURES_SEL, 0U);
  mmio_write32(device, BH_VIRTIO_MMIO_DRIVER_FEATURES, (uint32_t)agreed);
  mmio_write32(device, BH_VIRTIO_MMIO_DRIVER_FEATURES_SEL, 1U);
  mmio_write32(device, BH_VIRTIO_MMIO_DRIVER_FEATURES,
               (uint32_t)(agreed >> 32));

  status =
      mmio_read32(device, BH_VIRTIO_MMIO_STATUS) | BH_VIRTIO_STATUS_FEATURES_OK;
  mmio_write32(device, BH_VIRTIO_MMIO_STATUS, status);
  if ((mmio_read32(device, BH_VIRTIO_MMIO_STATUS) &
       BH_VIRTIO_STATUS_FEATURES_OK) == 0U) {
    set_failed(device);
    return BH_VIRTIO_MMIO_FEATURE_REJECTED;
  }

  device->driver_features = agreed;
  device->features_accepted = true;
  *negotiated_features = agreed;
  return BH_VIRTIO_MMIO_OK;
}

bh_virtio_mmio_result_t
bh_virtio_mmio_setup_queue(bh_virtio_mmio_device_t *device,
                           const bh_virtio_mmio_queue_t *queue) {
  uint32_t maximum;

  if (device == NULL || device->registers == NULL || queue == NULL ||
      !device->features_accepted || !is_power_of_two(queue->size) ||
      queue->descriptor_address == 0U || queue->available_address == 0U ||
      queue->used_address == 0U) {
    return BH_VIRTIO_MMIO_INVALID_ARGUMENT;
  }

  mmio_write32(device, BH_VIRTIO_MMIO_QUEUE_SEL, queue->index);
  maximum = mmio_read32(device, BH_VIRTIO_MMIO_QUEUE_NUM_MAX);
  if (maximum == 0U || queue->size > maximum) {
    return BH_VIRTIO_MMIO_QUEUE_UNAVAILABLE;
  }
  if (mmio_read32(device, BH_VIRTIO_MMIO_QUEUE_READY) != 0U) {
    return BH_VIRTIO_MMIO_QUEUE_BUSY;
  }

  mmio_write32(device, BH_VIRTIO_MMIO_QUEUE_NUM, queue->size);
  mmio_write64_pair(device, BH_VIRTIO_MMIO_QUEUE_DESC_LOW,
                    BH_VIRTIO_MMIO_QUEUE_DESC_HIGH, queue->descriptor_address);
  mmio_write64_pair(device, BH_VIRTIO_MMIO_QUEUE_AVAIL_LOW,
                    BH_VIRTIO_MMIO_QUEUE_AVAIL_HIGH, queue->available_address);
  mmio_write64_pair(device, BH_VIRTIO_MMIO_QUEUE_USED_LOW,
                    BH_VIRTIO_MMIO_QUEUE_USED_HIGH, queue->used_address);
  __atomic_thread_fence(__ATOMIC_RELEASE);
  mmio_write32(device, BH_VIRTIO_MMIO_QUEUE_READY, 1U);
  return BH_VIRTIO_MMIO_OK;
}

bh_virtio_mmio_result_t bh_virtio_mmio_start(bh_virtio_mmio_device_t *device) {
  uint32_t status;

  if (device == NULL || device->registers == NULL ||
      !device->features_accepted) {
    return BH_VIRTIO_MMIO_INVALID_ARGUMENT;
  }
  status = mmio_read32(device, BH_VIRTIO_MMIO_STATUS);
  if ((status & BH_VIRTIO_STATUS_FEATURES_OK) == 0U ||
      (status & BH_VIRTIO_STATUS_FAILED) != 0U) {
    set_failed(device);
    return BH_VIRTIO_MMIO_FEATURE_REJECTED;
  }
  mmio_write32(device, BH_VIRTIO_MMIO_STATUS,
               status | BH_VIRTIO_STATUS_DRIVER_OK);
  return BH_VIRTIO_MMIO_OK;
}

void bh_virtio_mmio_reset(bh_virtio_mmio_device_t *device) {
  if (device == NULL || device->registers == NULL) {
    return;
  }
  mmio_write32(device, BH_VIRTIO_MMIO_STATUS, 0U);
  __atomic_thread_fence(__ATOMIC_SEQ_CST);
  device->features_accepted = false;
  device->driver_features = 0U;
}

void bh_virtio_mmio_notify_queue(const bh_virtio_mmio_device_t *device,
                                 uint16_t queue_index) {
  if (device == NULL || device->registers == NULL) {
    return;
  }
  __atomic_thread_fence(__ATOMIC_RELEASE);
  mmio_write32(device, BH_VIRTIO_MMIO_QUEUE_NOTIFY, queue_index);
}
