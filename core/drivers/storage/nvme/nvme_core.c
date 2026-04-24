#include "nvme_core.h"
#include "nvme_regs.h"

// Define states
#define NVME_STATE_UNINIT 0
#define NVME_STATE_INIT 1

static inline uint32_t mmio_read32(volatile uint8_t *base, uint32_t offset) {
    return *(volatile uint32_t *)(base + offset);
}

static inline uint64_t mmio_read64(volatile uint8_t *base, uint32_t offset) {
    return *(volatile uint64_t *)(base + offset);
}

static inline void mmio_write32(volatile uint8_t *base, uint32_t offset, uint32_t value) {
    *(volatile uint32_t *)(base + offset) = value;
}

static inline void mmio_write64(volatile uint8_t *base, uint32_t offset, uint64_t value) {
    *(volatile uint64_t *)(base + offset) = value;
}

int nvme_controller_init(nvme_controller_t *ctrl, volatile uint8_t *mmio_base) {
    if (!ctrl || !mmio_base) return -1;

    ctrl->mmio_base = mmio_base;
    ctrl->state = NVME_STATE_UNINIT;

    // Basic verification - checking version register isn't all 0s or all 1s
    uint32_t vs = mmio_read32(mmio_base, NVME_REG_VS);
    if (vs == 0 || vs == 0xFFFFFFFF) {
        return -1; // Invalid MMIO region
    }

    // Check capability register
    uint64_t cap = mmio_read64(mmio_base, NVME_REG_CAP);
    if (cap == 0 || cap == 0xFFFFFFFFFFFFFFFF) {
        return -1; // Invalid CAP register
    }

    // Check if controller is currently enabled
    uint32_t cc = mmio_read32(mmio_base, NVME_REG_CC);
    if (cc & NVME_CC_EN) {
        // Disable controller
        mmio_write32(mmio_base, NVME_REG_CC, cc & ~NVME_CC_EN);

        // Wait for it to drop ready (mock delay here)
        uint32_t retries = 1000;
        while ((mmio_read32(mmio_base, NVME_REG_CSTS) & NVME_CSTS_RDY) && retries-- > 0) {
            // Mock waiting
        }
        if (retries == 0) return -1; // Timeout disabling
    }

    // Setup Admin Queue (stub logic)
    ctrl->admin_queue.id = 0;
    ctrl->admin_queue.size = 256; // Example mock size
    ctrl->admin_queue.sq_vaddr = (void *)0x1000; // Mock allocation
    ctrl->admin_queue.cq_vaddr = (void *)0x2000; // Mock allocation
    ctrl->admin_queue.sq_tail = 0;
    ctrl->admin_queue.cq_head = 0;

    mmio_write32(mmio_base, NVME_REG_AQA, (ctrl->admin_queue.size - 1) | ((ctrl->admin_queue.size - 1) << 16));
    mmio_write64(mmio_base, NVME_REG_ASQ, (uint64_t)(uintptr_t)ctrl->admin_queue.sq_vaddr); // mock paddr translation
    mmio_write64(mmio_base, NVME_REG_ACQ, (uint64_t)(uintptr_t)ctrl->admin_queue.cq_vaddr); // mock paddr translation

    // Enable controller
    mmio_write32(mmio_base, NVME_REG_CC, NVME_CC_EN);

    uint32_t retries = 1000;
    while (!(mmio_read32(mmio_base, NVME_REG_CSTS) & NVME_CSTS_RDY) && retries-- > 0) {
        // Mock waiting
    }
    if (retries == 0) return -1; // Timeout enabling

    ctrl->state = NVME_STATE_INIT;
    return 0; // Success
}

void nvme_controller_shutdown(nvme_controller_t *ctrl) {
    if (!ctrl || ctrl->state != NVME_STATE_INIT) return;

    uint32_t cc = mmio_read32(ctrl->mmio_base, NVME_REG_CC);
    mmio_write32(ctrl->mmio_base, NVME_REG_CC, cc & ~NVME_CC_EN);

    ctrl->state = NVME_STATE_UNINIT;
}