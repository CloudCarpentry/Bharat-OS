#include "drivers/dma/edu_dma.h"
#include "device/device_mmio.h"
#include "device/pci.h"
#include "hal/hal_dma.h"
#include "hal/hal_discovery.h"
#include "hal/hal.h"
#include "mm/dma.h"
#include "mm.h"
#include "capability.h"
#include "kernel/status.h"

extern capability_table_t *sched_current_cap_table(void);

#define EDU_POLL_TIMEOUT_CYCLES  5000000U

static bh_edu_dma_device_t g_primary_edu_dev = {0};

static inline uint32_t edu_read32(const bh_edu_dma_device_t *dev, uint32_t offset) {
    return *(volatile uint32_t *)(dev->mmio_virt + offset);
}

static inline void edu_write32(const bh_edu_dma_device_t *dev, uint32_t offset, uint32_t val) {
    *(volatile uint32_t *)(dev->mmio_virt + offset) = val;
}

static inline uint64_t edu_read64(const bh_edu_dma_device_t *dev, uint32_t offset) {
    uint32_t low = *(volatile uint32_t *)(dev->mmio_virt + offset);
    uint32_t high = *(volatile uint32_t *)(dev->mmio_virt + offset + 4);
    return ((uint64_t)high << 32) | low;
}

static inline void edu_write64(const bh_edu_dma_device_t *dev, uint32_t offset, uint64_t val) {
    *(volatile uint32_t *)(dev->mmio_virt + offset) = (uint32_t)(val & 0xFFFFFFFFU);
    *(volatile uint32_t *)(dev->mmio_virt + offset + 4) = (uint32_t)(val >> 32);
}

kstatus_t bh_edu_dma_verify_liveness(bh_edu_dma_device_t *dev) {
    if (!dev || !dev->mmio_virt) {
        return K_ERR_INVALID_ARG;
    }

    const uint32_t test_pattern = 0x55AA55AAU;
    edu_write32(dev, BH_EDU_REG_LIVENESS, test_pattern);
    uint32_t inverted = edu_read32(dev, BH_EDU_REG_LIVENESS);

    if (inverted != ~test_pattern) {
        return K_ERR_DEV_MMIO_FAULT;
    }
    return K_OK;
}

kstatus_t bh_edu_dma_probe(pci_device_t *pci, bh_edu_dma_device_t *out_dev) {
    if (!pci || !out_dev) {
        return K_ERR_INVALID_ARG;
    }

    if (pci->vendor_id != BH_EDU_PCI_VENDOR_ID || pci->device_id != BH_EDU_PCI_DEVICE_ID) {
        return K_ERR_DEV_NO_DEVICE;
    }

    /* Check if BAR0 has been programmed with a valid physical base */
    uint64_t bar0_phys = (uint64_t)(pci->bar[0] & ~0xFU);
    size_t bar0_size = pci->bar_size[0];

    hal_serial_write("BH_DMA_HW:RAW_BAR0=0x");
    hal_serial_write_hex(bar0_phys);
    hal_serial_write(" SIZE=0x");
    hal_serial_write_hex((uint64_t)bar0_size);
    hal_serial_write("\n");

    if (bar0_phys == 0U) {
        /* Unprogrammed BAR0 in QEMU: probe size if not already probed */
        if (bar0_size == 0U) {
            uint32_t orig_bar = pci_config_read32(pci, 0x10);
            pci_config_write32(pci, 0x10, 0xFFFFFFFFU);
            uint32_t mask = pci_config_read32(pci, 0x10);
            pci_config_write32(pci, 0x10, orig_bar);
            if (mask != 0U && mask != 0xFFFFFFFFU) {
                bar0_size = (size_t)(~(mask & ~0xFU) + 1U);
            } else {
                bar0_size = 0x100000U; /* Standard 1MB size for EDU BAR0 */
            }
            pci->bar_size[0] = (uint32_t)bar0_size;
        }

        /* Assign physical address from PCIe host MMIO window */
        system_discovery_t *disc = hal_get_system_discovery();
        uint64_t assigned_base = 0U;
        if (disc && disc->pci_host_count > 0 && disc->pci_hosts[0].mmio32_base != 0U) {
            assigned_base = disc->pci_hosts[0].mmio32_base;
        } else {
#if defined(__x86_64__)
            assigned_base = 0xFEBF0000ULL;
#elif defined(__aarch64__)
            assigned_base = 0x10000000ULL;
#elif defined(__riscv)
            assigned_base = 0x40000000ULL;
#else
            assigned_base = 0xC0000000ULL;
#endif
        }

        pci->bar[0] = (uint32_t)assigned_base;
        pci_config_write32(pci, 0x10, (uint32_t)assigned_base);
        bar0_phys = assigned_base;
        hal_serial_write("BH_DMA_HW:ASSIGNED_BAR0=0x");
        hal_serial_write_hex(bar0_phys);
        hal_serial_write("\n");
    }

    if (bar0_size == 0U) {
        bar0_size = 0x100000U;
    }

    /* Enable Bus Mastering and Memory Space in PCI Command register */
    if (pci_enable_device(pci) != 0) {
        hal_serial_write("BH_DMA_HW:FAIL_ENABLE_DEVICE\n");
        return K_ERR_DEV_NOT_READY;
    }

    /* Map BAR0 into kernel virtual memory */
    void *virt_mmio = NULL;
    int map_ret = bh_device_mmio_map(bar0_phys, bar0_size, 0U, &virt_mmio);
    if (map_ret != 0 || !virt_mmio) {
        hal_serial_write("BH_DMA_HW:FAIL_MMIO_MAP ret=");
        hal_serial_write_hex((uint64_t)map_ret);
        hal_serial_write("\n");
        return K_ERR_DEV_MMIO_FAULT;
    }

    out_dev->pci_dev = pci;
    out_dev->mmio_phys = bar0_phys;
    out_dev->mmio_size = bar0_size;
    out_dev->mmio_virt = (volatile uint8_t *)virt_mmio;

    /* Verify device identification register */
    uint32_t dev_id_reg = edu_read32(out_dev, BH_EDU_REG_ID);
    hal_serial_write("BH_DMA_HW:DEV_ID_REG=0x");
    hal_serial_write_hex((uint64_t)dev_id_reg);
    hal_serial_write("\n");

    /* Lower byte is 0xed, upper byte is major version (0x01) */
    if (dev_id_reg != 0x010000edU && dev_id_reg != 0xed000001U) {
        hal_serial_write("BH_DMA_HW:FAIL_MAGIC_MISMATCH\n");
        return K_ERR_DEV_NO_DEVICE;
    }
    out_dev->hw_version = dev_id_reg;

    /* Verify hardware register write and readback inversion */
    kstatus_t live_status = bh_edu_dma_verify_liveness(out_dev);
    if (live_status != K_OK) {
        hal_serial_write("BH_DMA_HW:FAIL_LIVENESS ret=");
        hal_serial_write_hex((uint64_t)live_status);
        hal_serial_write("\n");
        return live_status;
    }

    out_dev->initialized = true;
    g_primary_edu_dev = *out_dev;

    hal_serial_write("BH_DMA_HW:EDU_DEV_FOUND [0x1234:0x11e8] BAR0=0x");
    hal_serial_write_hex(bar0_phys);
    hal_serial_write(" LIVENESS=PASS\n");

    return K_OK;
}

kstatus_t bh_edu_dma_transfer(bh_edu_dma_device_t *dev,
                              uint64_t src_phys,
                              uint64_t dst_phys,
                              size_t size) {
    if (!dev || !dev->initialized || !dev->mmio_virt) {
        return K_ERR_BAD_STATE;
    }
    if (size == 0U || src_phys == 0U || dst_phys == 0U) {
        return K_ERR_INVALID_ARG;
    }

    size_t transferred = 0U;
    while (transferred < size) {
        size_t chunk = size - transferred;
        if (chunk > BH_EDU_MAX_DMA_CHUNK) {
            chunk = BH_EDU_MAX_DMA_CHUNK;
        }

        uint64_t cur_src = src_phys + (uint64_t)transferred;
        uint64_t cur_dst = dst_phys + (uint64_t)transferred;

        /* --- Hop 1: RAM -> EDU Device SRAM Buffer --- */
        edu_write64(dev, BH_EDU_REG_DMA_SRC, cur_src);
        edu_write64(dev, BH_EDU_REG_DMA_DST, (uint64_t)BH_EDU_BUFFER_OFFSET);
        edu_write64(dev, BH_EDU_REG_DMA_CNT, (uint64_t)chunk);
        edu_write64(dev, BH_EDU_REG_DMA_CMD, BH_EDU_CMD_START);

        uint32_t poll_count = 0U;
        while ((edu_read64(dev, BH_EDU_REG_DMA_CMD) & BH_EDU_CMD_START) != 0ULL) {
            poll_count++;
            if (poll_count >= EDU_POLL_TIMEOUT_CYCLES) {
                return K_ERR_TIMEOUT;
            }
        }

        /* --- Hop 2: EDU Device SRAM Buffer -> RAM --- */
        edu_write64(dev, BH_EDU_REG_DMA_SRC, (uint64_t)BH_EDU_BUFFER_OFFSET);
        edu_write64(dev, BH_EDU_REG_DMA_DST, cur_dst);
        edu_write64(dev, BH_EDU_REG_DMA_CNT, (uint64_t)chunk);
        edu_write64(dev, BH_EDU_REG_DMA_CMD, BH_EDU_CMD_START | BH_EDU_CMD_DIR_TO_RAM);

        poll_count = 0U;
        while ((edu_read64(dev, BH_EDU_REG_DMA_CMD) & BH_EDU_CMD_START) != 0ULL) {
            poll_count++;
            if (poll_count >= EDU_POLL_TIMEOUT_CYCLES) {
                return K_ERR_TIMEOUT;
            }
        }

        transferred += chunk;
    }

    return K_OK;
}

bh_edu_dma_device_t *bh_edu_dma_get_primary(void) {
    if (!g_primary_edu_dev.initialized) {
        return NULL;
    }
    return &g_primary_edu_dev;
}

kstatus_t bh_edu_dma_init(void) {
    if (g_primary_edu_dev.initialized) {
        return K_OK;
    }

    hal_serial_write("BH_DMA_HW:INIT_SCANNING_PCI...\n");
    int count = pci_enumerate();
    hal_serial_write("BH_DMA_HW:PCI_DEV_COUNT=");
    hal_serial_write_hex((uint64_t)count);
    hal_serial_write("\n");

    pci_device_t *pci = pci_get_device_list();
    while (pci != NULL) {
        hal_serial_write("BH_DMA_HW:PCI_DEV [0x");
        hal_serial_write_hex(pci->vendor_id);
        hal_serial_write(":0x");
        hal_serial_write_hex(pci->device_id);
        hal_serial_write("]\n");

        if (pci->vendor_id == BH_EDU_PCI_VENDOR_ID &&
            pci->device_id == BH_EDU_PCI_DEVICE_ID) {
            kstatus_t status = bh_edu_dma_probe(pci, &g_primary_edu_dev);
            if (status == K_OK) {
                return K_OK;
            }
        }
        pci = pci->next;
    }

    hal_serial_write("BH_DMA_HW:NOT_FOUND\n");
    return K_ERR_NOT_FOUND;
}

kstatus_t bh_edu_dma_self_test(void) {
    hal_serial_write("BH_DMA_HW:SELF_TEST_ENTER\n");
    bh_edu_dma_device_t *dev = bh_edu_dma_get_primary();
    if (!dev) {
        kstatus_t init_st = bh_edu_dma_init();
        if (init_st != K_OK) {
            hal_serial_write("BH_DMA_HW:INIT_FAILED status=");
            hal_serial_write_hex((uint64_t)init_st);
            hal_serial_write("\n");
            return init_st;
        }
        dev = bh_edu_dma_get_primary();
        if (!dev) {
            return K_ERR_NOT_FOUND;
        }
    }

    const size_t test_size = 1024U;

    /* 1. Allocate DMA-capable contiguous physical buffers via PMM */
    dma_buffer_t *src_buf = NULL;
    dma_buffer_t *dst_buf = NULL;

    hal_serial_write("BH_DMA_HW:ALLOC_BUFFERS...\n");
    int a1 = dma_buffer_alloc(test_size, DMA_ALLOC_ZERO | DMA_ALLOC_DMA32, &src_buf);
    int a2 = dma_buffer_alloc(test_size, DMA_ALLOC_ZERO | DMA_ALLOC_DMA32, &dst_buf);
    if (a1 != 0 || !src_buf || a2 != 0 || !dst_buf) {
        hal_serial_write("BH_DMA_HW:ALLOC_FAILED a1=");
        hal_serial_write_hex((uint64_t)a1);
        hal_serial_write(" a2=");
        hal_serial_write_hex((uint64_t)a2);
        hal_serial_write("\n");
        if (src_buf) dma_buffer_free(src_buf);
        if (dst_buf) dma_buffer_free(dst_buf);
        return K_ERR_NO_MEMORY;
    }
    hal_serial_write("BH_DMA_HW:BUFFERS_ALLOCATED SRC=0x");
    hal_serial_write_hex(src_buf->phys_addr);
    hal_serial_write(" DST=0x");
    hal_serial_write_hex(dst_buf->phys_addr);
    hal_serial_write("\n");

    /* 2. Populate source buffer with a deterministic non-zero pattern */
    uint8_t *src_data = (uint8_t *)src_buf->cpu_addr;
    uint8_t *dst_data = (uint8_t *)dst_buf->cpu_addr;
    for (size_t i = 0; i < test_size; ++i) {
        src_data[i] = (uint8_t)((i * 17U + 0x5AU) & 0xFFU);
        dst_data[i] = 0x00U;
    }

    /* 3. Pin both buffers */
    if (dma_buffer_pin(src_buf, 1U) != 0 || dma_buffer_pin(dst_buf, 1U) != 0) {
        hal_serial_write("BH_DMA_HW:PIN_FAILED\n");
        dma_buffer_free(dst_buf);
        dma_buffer_free(src_buf);
        return K_ERR_BAD_STATE;
    }

    /* 4. Obtain DMA grant capabilities if capability table is present */
    capability_table_t *ct = sched_current_cap_table();
    uint32_t src_cap = 0U;
    uint32_t dst_cap = 0U;
    if (ct) {
        (void)cap_table_grant(ct, CAP_TYPE_DMA_GRANT, (uint64_t)src_buf,
                              CAP_RIGHT_DMA_MAP | CAP_RIGHT_MEMORY_UNMAP, &src_cap);
        (void)cap_table_grant(ct, CAP_TYPE_DMA_GRANT, (uint64_t)dst_buf,
                              CAP_RIGHT_DMA_MAP | CAP_RIGHT_MEMORY_UNMAP, &dst_cap);
    }

    int m1 = dma_buffer_map_device((uint64_t)src_cap, src_buf, DMA_MAP_TO_DEVICE);
    int m2 = dma_buffer_map_device((uint64_t)dst_cap, dst_buf, DMA_MAP_FROM_DEVICE);
    if (m1 != 0 || m2 != 0) {
        hal_serial_write("BH_DMA_HW:MAP_FAILED m1=");
        hal_serial_write_hex((uint64_t)m1);
        hal_serial_write(" m2=");
        hal_serial_write_hex((uint64_t)m2);
        hal_serial_write("\n");
        if (ct) {
            if (dst_cap != 0U) (void)cap_table_revoke(ct, dst_cap);
            if (src_cap != 0U) (void)cap_table_revoke(ct, src_cap);
        }
        dma_buffer_unpin(dst_buf);
        dma_buffer_unpin(src_buf);
        dma_buffer_free(dst_buf);
        dma_buffer_free(src_buf);
        return K_ERR_BAD_STATE;
    }

    /* 5. Clean cache for source buffer before device DMA reads it */
    dma_sync_for_device(src_buf, DMA_MAP_TO_DEVICE);

    /* 6. Execute authentic hardware DMA transfer */
    hal_serial_write("BH_DMA_HW:EXECUTING_TRANSFER...\n");
    kstatus_t xfer_status = bh_edu_dma_transfer(dev,
                                                src_buf->phys_addr,
                                                dst_buf->phys_addr,
                                                test_size);
    if (xfer_status != K_OK) {
        hal_serial_write("BH_DMA_HW:TRANSFER_FAILED status=");
        hal_serial_write_hex((uint64_t)xfer_status);
        hal_serial_write("\n");
        dma_buffer_unmap_device((uint64_t)dst_cap, dst_buf, DMA_MAP_FROM_DEVICE);
        dma_buffer_unmap_device((uint64_t)src_cap, src_buf, DMA_MAP_TO_DEVICE);
        if (ct) {
            if (dst_cap != 0U) (void)cap_table_revoke(ct, dst_cap);
            if (src_cap != 0U) (void)cap_table_revoke(ct, src_cap);
        }
        dma_buffer_unpin(dst_buf);
        dma_buffer_unpin(src_buf);
        dma_buffer_free(dst_buf);
        dma_buffer_free(src_buf);
        return xfer_status;
    }

    /* 7. Invalidate cache for destination buffer after device DMA wrote to it */
    dma_sync_for_cpu(dst_buf, DMA_MAP_FROM_DEVICE);

    /* 8. Byte-exact destination RAM buffer validation */
    bool dst_verified = true;
    for (size_t i = 0; i < test_size; ++i) {
        if (dst_data[i] != src_data[i]) {
            dst_verified = false;
            break;
        }
    }

    /* 9. Clean up and unmap all resources safely */
    dma_buffer_unmap_device((uint64_t)dst_cap, dst_buf, DMA_MAP_FROM_DEVICE);
    dma_buffer_unmap_device((uint64_t)src_cap, src_buf, DMA_MAP_TO_DEVICE);
    if (ct) {
        if (dst_cap != 0U) (void)cap_table_revoke(ct, dst_cap);
        if (src_cap != 0U) (void)cap_table_revoke(ct, src_cap);
    }
    dma_buffer_unpin(dst_buf);
    dma_buffer_unpin(src_buf);
    dma_buffer_free(dst_buf);
    dma_buffer_free(src_buf);

    if (!dst_verified) {
        hal_serial_write("BH_DMA_HW:FAIL=CONTENT_MISMATCH\n");
        return K_ERR_CHECKSUM;
    }

    hal_serial_write("BH_DMA_HW:TRANSFER=PASS BYTES=1024 REG_CHECK=PASS\n");
    return K_OK;
}
