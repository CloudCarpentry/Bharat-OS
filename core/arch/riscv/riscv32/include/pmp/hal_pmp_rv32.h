#ifndef BHARAT_HAL_PMP_RV32_H
#define BHARAT_HAL_PMP_RV32_H

/* Architecture-specific RV32 PMP CSR definitions */

#define RV32_CSR_PMPCFG0   0x3A0
#define RV32_CSR_PMPCFG1   0x3A1
#define RV32_CSR_PMPCFG2   0x3A2
#define RV32_CSR_PMPCFG3   0x3A3

#define RV32_CSR_PMPADDR0  0x3B0

#define RV32_PMP_R         (1u << 0)
#define RV32_PMP_W         (1u << 1)
#define RV32_PMP_X         (1u << 2)

#define RV32_PMP_A_OFF     (0u << 3)
#define RV32_PMP_A_TOR     (1u << 3)
#define RV32_PMP_A_NA4     (2u << 3)
#define RV32_PMP_A_NAPOT   (3u << 3)
#define RV32_PMP_A_MASK    (3u << 3)

#define RV32_PMP_L         (1u << 7)

#endif // BHARAT_HAL_PMP_RV32_H
