#ifndef BHARAT_SBI_H
#define BHARAT_SBI_H

#include <stdint.h>

/*
 * Bharat-OS RISC-V Supervisor Binary Interface (SBI)
 * Since RISC-V lacks a standard PC-BIOS, the SBI firmware (like OpenSBI)
 * boots the kernel in Supervisor Mode (S-Mode) and provides these system calls.
 */

#define SBI_EXT_TIME 0x54494D45
#define SBI_EXT_IPI  0x735049
#define SBI_EXT_RFNC 0x52464E43

// SBI Return Structure
typedef struct {
    long error;
    long value;
} sbi_ret_t;

// Inline assembly to trigger an `ecall` to the RISC-V Machine Mode (M-Mode) firmware
static inline sbi_ret_t sbi_call(long ext, long fid, long arg0, long arg1, long arg2) {
    register long a0 asm("a0") = arg0;
    register long a1 asm("a1") = arg1;
    register long a2 asm("a2") = arg2;
    register long a6 asm("a6") = fid;
    register long a7 asm("a7") = ext;
    
    asm volatile(
        "ecall"
        : "+r"(a0), "+r"(a1)
        : "r"(a2), "r"(a6), "r"(a7)
        : "memory"
    );
    
    sbi_ret_t ret = {a0, a1};
    return ret;
}

// Request the firmware to set the next timer interrupt
static inline void sbi_set_timer(uint64_t stime_value) {
    sbi_call(SBI_EXT_TIME, 0, stime_value, 0, 0);
}

// Send an Inter-Processor Interrupt (IPI) to wake up other CPU cores
static inline void sbi_send_ipi(const unsigned long* hart_mask) {
    sbi_call(SBI_EXT_IPI, 0, (long)hart_mask, 0, 0);
}

// Send an IPI with payload (Assuming a custom SBI extension or future standard)
#define SBI_EXT_IPI_PAYLOAD 0x73504A

static inline void sbi_send_ipi_payload(const unsigned long* hart_mask, uint64_t payload) {
    sbi_call(SBI_EXT_IPI_PAYLOAD, 0, (long)hart_mask, (long)payload, 0);
}

// Kernel Entry point launched by OpenSBI
void kernel_main(uint64_t hartid, uint64_t device_tree_ptr);

#endif // BHARAT_SBI_H
