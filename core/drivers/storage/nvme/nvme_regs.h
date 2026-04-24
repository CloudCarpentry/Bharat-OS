#ifndef BHARATOS_NVME_REGS_H
#define BHARATOS_NVME_REGS_H

#include <stdint.h>

// NVMe MMIO Register Offsets
#define NVME_REG_CAP    0x00 // Controller Capabilities
#define NVME_REG_VS     0x08 // Version
#define NVME_REG_INTMS  0x0c // Interrupt Mask Set
#define NVME_REG_INTMC  0x10 // Interrupt Mask Clear
#define NVME_REG_CC     0x14 // Controller Configuration
#define NVME_REG_CSTS   0x1c // Controller Status
#define NVME_REG_AQA    0x24 // Admin Queue Attributes
#define NVME_REG_ASQ    0x28 // Admin Submission Queue Base Address
#define NVME_REG_ACQ    0x30 // Admin Completion Queue Base Address

// Controller Configuration (CC) Bits
#define NVME_CC_EN      (1 << 0)  // Enable
#define NVME_CSTS_RDY   (1 << 0)  // Ready

#endif /* BHARATOS_NVME_REGS_H */