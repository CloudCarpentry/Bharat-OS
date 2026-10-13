#ifndef BHARAT_DRIVERS_DMA_EDU_DMA_H
#define BHARAT_DRIVERS_DMA_EDU_DMA_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "device/pci.h"
#include "kernel/status.h"

#ifdef __cplusplus
extern "C" {
#endif

/* QEMU EDU PCI device identification */
#define BH_EDU_PCI_VENDOR_ID     0x1234U
#define BH_EDU_PCI_DEVICE_ID     0x11e8U

/* EDU MMIO Register Offsets */
#define BH_EDU_REG_ID            0x00U  /* 32-bit: identification / version */
#define BH_EDU_REG_LIVENESS      0x04U  /* 32-bit: invert test (write val -> read ~val) */
#define BH_EDU_REG_FACTORIAL     0x08U  /* 32-bit: factorial computation */
#define BH_EDU_REG_STATUS        0x20U  /* 32-bit: device status */
#define BH_EDU_REG_IRQ_STATUS    0x24U  /* 32-bit: interrupt status */
#define BH_EDU_REG_IRQ_ACK       0x64U  /* 32-bit: interrupt acknowledge */
#define BH_EDU_REG_DMA_SRC       0x80U  /* 64-bit: DMA source RAM physical address */
#define BH_EDU_REG_DMA_DST       0x88U  /* 64-bit: DMA destination RAM physical address */
#define BH_EDU_REG_DMA_CNT       0x90U  /* 64-bit: transfer size in bytes */
#define BH_EDU_REG_DMA_CMD       0x98U  /* 64-bit: DMA command & busy status */

/* Internal 4096-byte hardware SRAM buffer */
#define BH_EDU_BUFFER_OFFSET     0x40000U
#define BH_EDU_MAX_DMA_CHUNK     4096U

/* DMA Command Bitfields */
#define BH_EDU_CMD_START         0x01ULL /* Start transfer (read: 1=busy, 0=idle) */
#define BH_EDU_CMD_DIR_TO_RAM    0x02ULL /* Direction: 0=RAM->EDU, 1=EDU->RAM */
#define BH_EDU_CMD_IRQ_ENABLE    0x04ULL /* Raise interrupt upon completion */

/* Expected hardware magic in BH_EDU_REG_ID: version 0x01, id 0xed (0x010000ed) */
#define BH_EDU_MAGIC_ID_VAL      0x010000edU

typedef struct bh_edu_dma_device {
    pci_device_t *pci_dev;
    uint64_t mmio_phys;
    size_t mmio_size;
    volatile uint8_t *mmio_virt;
    bool initialized;
    uint32_t hw_version;
} bh_edu_dma_device_t;

/**
 * Probes and initializes the EDU DMA controller from a discovered PCI device.
 *
 * @param pci The discovered PCI device structure.
 * @param out_dev Output structure to populate with initialized state.
 * @return K_OK on success, or a canonical negative error status.
 */
kstatus_t bh_edu_dma_probe(pci_device_t *pci, bh_edu_dma_device_t *out_dev);

/**
 * Executes a hardware memory-to-memory DMA transfer between physical addresses.
 * Data is routed through the device's internal SRAM engine using hardware bus-mastering.
 *
 * @param dev The initialized EDU DMA controller.
 * @param src_phys Source physical memory address.
 * @param dst_phys Destination physical memory address.
 * @param size Number of bytes to transfer.
 * @return K_OK on success, or a canonical negative error status.
 */
kstatus_t bh_edu_dma_transfer(bh_edu_dma_device_t *dev,
                              uint64_t src_phys,
                              uint64_t dst_phys,
                              size_t size);

/**
 * Verifies hardware device communication via the invert test register.
 *
 * @param dev The initialized EDU DMA controller.
 * @return K_OK if the hardware properly inverted the test pattern, or K_ERR_DEV_MMIO_FAULT.
 */
kstatus_t bh_edu_dma_verify_liveness(bh_edu_dma_device_t *dev);

/**
 * Returns the primary registered EDU DMA controller instance, or NULL if none.
 */
bh_edu_dma_device_t *bh_edu_dma_get_primary(void);

/**
 * Auto-detects and initializes the EDU DMA controller across all scanned PCI devices.
 *
 * @return K_OK on success, K_ERR_NOT_FOUND if no EDU device is present on the bus.
 */
kstatus_t bh_edu_dma_init(void);

/**
 * Comprehensive end-to-end self-test of the EDU DMA controller using
 * Bharat-OS HAL DMA contiguous buffers, cache coherency synchronization,
 * hardware completion polling, and byte-exact validation.
 *
 * @return K_OK on success, or a negative failure code.
 */
kstatus_t bh_edu_dma_self_test(void);

#ifdef __cplusplus
}
#endif

#endif /* BHARAT_DRIVERS_DMA_EDU_DMA_H */
