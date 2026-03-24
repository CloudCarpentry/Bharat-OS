#ifndef BHARATOS_NVME_MOCK_H
#define BHARATOS_NVME_MOCK_H

#include <stdint.h>
#include "nvme_regs.h"

// Define a structure representing the mock MMIO registers
typedef struct {
    uint64_t cap;
    uint32_t vs;
    uint32_t intms;
    uint32_t intmc;
    uint32_t cc;
    uint32_t csts;
    uint32_t nssr;
    uint32_t aqa;
    uint64_t asq;
    uint64_t acq;
} nvme_mock_mmio_t;

// Set up mock NVMe state, allowing tests to emulate hardware register responses
void nvme_mock_mmio_init(nvme_mock_mmio_t *mmio);
void nvme_mock_mmio_set_ready(nvme_mock_mmio_t *mmio, int ready);
void nvme_mock_mmio_set_invalid(nvme_mock_mmio_t *mmio);

#endif /* BHARATOS_NVME_MOCK_H */