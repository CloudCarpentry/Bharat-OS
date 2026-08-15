#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "virtio_mmio.h"

#define REG_MAGIC 0x000U
#define REG_VERSION 0x004U
#define REG_DEVICE_ID 0x008U
#define REG_VENDOR_ID 0x00cU
#define REG_DEVICE_FEATURES 0x010U
#define REG_QUEUE_NUM_MAX 0x034U
#define REG_QUEUE_NUM 0x038U
#define REG_QUEUE_READY 0x044U
#define REG_QUEUE_NOTIFY 0x050U
#define REG_STATUS 0x070U
#define REG_QUEUE_DESC_LOW 0x080U
#define REG_QUEUE_DESC_HIGH 0x084U

static void write_reg(uint8_t *registers, uint32_t offset, uint32_t value) {
  memcpy(registers + offset, &value, sizeof(value));
}

static uint32_t read_reg(const uint8_t *registers, uint32_t offset) {
  uint32_t value;
  memcpy(&value, registers + offset, sizeof(value));
  return value;
}

static void make_device(uint8_t *registers) {
  memset(registers, 0, 0x100U);
  write_reg(registers, REG_MAGIC, BH_VIRTIO_MMIO_MAGIC);
  write_reg(registers, REG_VERSION, BH_VIRTIO_MMIO_VERSION_MODERN);
  write_reg(registers, REG_DEVICE_ID, 2U);
  write_reg(registers, REG_VENDOR_ID, 0x1af4U);
  /* The simple register model returns bit zero for both selected banks. */
  write_reg(registers, REG_DEVICE_FEATURES, 1U);
  write_reg(registers, REG_QUEUE_NUM_MAX, 64U);
}

static void test_probe_rejects_invalid_windows(void) {
  _Alignas(uint32_t) uint8_t registers[0x100U];
  bh_virtio_mmio_device_t device;

  memset(registers, 0, sizeof(registers));
  assert(bh_virtio_mmio_probe(&device, registers) == BH_VIRTIO_MMIO_NOT_DEVICE);
  make_device(registers);
  write_reg(registers, REG_VERSION, 1U);
  assert(bh_virtio_mmio_probe(&device, registers) ==
         BH_VIRTIO_MMIO_UNSUPPORTED);
}

static void test_feature_failure_is_fail_closed(void) {
  _Alignas(uint32_t) uint8_t registers[0x100U];
  bh_virtio_mmio_device_t device;
  uint64_t negotiated = 0U;

  make_device(registers);
  assert(bh_virtio_mmio_probe(&device, registers) == BH_VIRTIO_MMIO_OK);
  assert(bh_virtio_mmio_negotiate_features(
             &device, BH_VIRTIO_F_VERSION_1, BH_VIRTIO_F_VERSION_1 | 2U,
             &negotiated) == BH_VIRTIO_MMIO_FEATURE_REJECTED);
  assert((read_reg(registers, REG_STATUS) & BH_VIRTIO_STATUS_FAILED) != 0U);
  assert(!device.features_accepted);
}

static void test_queue_lifecycle(void) {
  _Alignas(uint32_t) uint8_t registers[0x100U];
  bh_virtio_mmio_device_t device;
  uint64_t negotiated = 0U;
  const bh_virtio_mmio_queue_t queue = {
      .index = 3U,
      .size = 16U,
      .descriptor_address = UINT64_C(0x123456788000),
      .available_address = UINT64_C(0x123456789000),
      .used_address = UINT64_C(0x12345678a000),
  };

  make_device(registers);
  assert(bh_virtio_mmio_probe(&device, registers) == BH_VIRTIO_MMIO_OK);
  assert(bh_virtio_mmio_negotiate_features(&device, BH_VIRTIO_F_VERSION_1 | 1U,
                                           BH_VIRTIO_F_VERSION_1,
                                           &negotiated) == BH_VIRTIO_MMIO_OK);
  assert(negotiated == (BH_VIRTIO_F_VERSION_1 | 1U));
  assert(bh_virtio_mmio_setup_queue(&device, &queue) == BH_VIRTIO_MMIO_OK);
  assert(read_reg(registers, REG_QUEUE_NUM) == queue.size);
  assert(read_reg(registers, REG_QUEUE_DESC_LOW) ==
         (uint32_t)queue.descriptor_address);
  assert(read_reg(registers, REG_QUEUE_DESC_HIGH) ==
         (uint32_t)(queue.descriptor_address >> 32));
  assert(read_reg(registers, REG_QUEUE_READY) == 1U);

  assert(bh_virtio_mmio_setup_queue(&device, &queue) ==
         BH_VIRTIO_MMIO_QUEUE_BUSY);
  assert(bh_virtio_mmio_start(&device) == BH_VIRTIO_MMIO_OK);
  assert((read_reg(registers, REG_STATUS) & BH_VIRTIO_STATUS_DRIVER_OK) != 0U);
  bh_virtio_mmio_notify_queue(&device, queue.index);
  assert(read_reg(registers, REG_QUEUE_NOTIFY) == queue.index);
  bh_virtio_mmio_reset(&device);
  assert(read_reg(registers, REG_STATUS) == 0U);
  assert(!device.features_accepted);
}

int main(void) {
  test_probe_rejects_invalid_windows();
  test_feature_failure_is_fail_closed();
  test_queue_lifecycle();
  puts("VirtIO MMIO transport tests passed");
  return 0;
}
