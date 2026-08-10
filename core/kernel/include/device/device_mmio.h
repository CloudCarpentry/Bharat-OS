#ifndef BHARAT_DEVICE_MMIO_H
#define BHARAT_DEVICE_MMIO_H

#include <stdint.h>
#include <stddef.h>

/**
 * Maps a physical MMIO address range to kernel virtual space safely.
 * Reuses physmap linear mapping or explicitly programs the page table.
 *
 * @param phys The physical address base of the device region.
 * @param size The size of the region in bytes.
 * @param attributes Extra page table attributes/flags (e.g., HAL_PT_FLAG_DEVICE).
 * @param out_virt Pointer to receive the mapped kernel virtual address.
 * @return 0 on success, negative error code on failure.
 */
int bh_device_mmio_map(uint64_t phys,
                       size_t size,
                       uint32_t attributes,
                       void **out_virt);

#endif // BHARAT_DEVICE_MMIO_H
