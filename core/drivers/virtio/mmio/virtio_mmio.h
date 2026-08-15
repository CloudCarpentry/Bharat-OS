#ifndef BHARAT_VIRTIO_MMIO_H
#define BHARAT_VIRTIO_MMIO_H

#include <stdbool.h>
#include <stdint.h>

#define BH_VIRTIO_MMIO_MAGIC UINT32_C(0x74726976)
#define BH_VIRTIO_MMIO_VERSION_MODERN UINT32_C(2)
#define BH_VIRTIO_F_VERSION_1 (UINT64_C(1) << 32)

#define BH_VIRTIO_STATUS_ACKNOWLEDGE UINT32_C(1)
#define BH_VIRTIO_STATUS_DRIVER UINT32_C(2)
#define BH_VIRTIO_STATUS_DRIVER_OK UINT32_C(4)
#define BH_VIRTIO_STATUS_FEATURES_OK UINT32_C(8)
#define BH_VIRTIO_STATUS_FAILED UINT32_C(128)

typedef enum bh_virtio_mmio_result {
  BH_VIRTIO_MMIO_OK = 0,
  BH_VIRTIO_MMIO_INVALID_ARGUMENT = -1,
  BH_VIRTIO_MMIO_NOT_DEVICE = -2,
  BH_VIRTIO_MMIO_UNSUPPORTED = -3,
  BH_VIRTIO_MMIO_FEATURE_REJECTED = -4,
  BH_VIRTIO_MMIO_QUEUE_UNAVAILABLE = -5,
  BH_VIRTIO_MMIO_QUEUE_BUSY = -6,
} bh_virtio_mmio_result_t;

/*
 * One driver domain owns and serializes each instance. The register window is
 * shared with the device, but this structure is never sent over IPC and holds
 * no service policy.
 */
typedef struct bh_virtio_mmio_device {
  volatile uint8_t *registers;
  uint32_t device_id;
  uint32_t vendor_id;
  uint64_t device_features;
  uint64_t driver_features;
  bool features_accepted;
} bh_virtio_mmio_device_t;

typedef struct bh_virtio_mmio_queue {
  uint16_t index;
  uint16_t size;
  uint64_t descriptor_address;
  uint64_t available_address;
  uint64_t used_address;
} bh_virtio_mmio_queue_t;

bh_virtio_mmio_result_t bh_virtio_mmio_probe(bh_virtio_mmio_device_t *device,
                                             volatile void *registers);
bh_virtio_mmio_result_t bh_virtio_mmio_negotiate_features(
    bh_virtio_mmio_device_t *device, uint64_t supported_features,
    uint64_t required_features, uint64_t *negotiated_features);
bh_virtio_mmio_result_t
bh_virtio_mmio_setup_queue(bh_virtio_mmio_device_t *device,
                           const bh_virtio_mmio_queue_t *queue);
bh_virtio_mmio_result_t bh_virtio_mmio_start(bh_virtio_mmio_device_t *device);
void bh_virtio_mmio_reset(bh_virtio_mmio_device_t *device);
void bh_virtio_mmio_notify_queue(const bh_virtio_mmio_device_t *device,
                                 uint16_t queue_index);

#endif
