#ifndef BHARAT_SBI_H
#define BHARAT_SBI_H

#include <stdint.h>

/*
 * Bharat-OS RISC-V Supervisor Binary Interface (SBI)
 * Since RISC-V lacks a standard PC-BIOS, the SBI firmware (like OpenSBI)
 * boots the kernel in Supervisor Mode (S-Mode) and provides these system calls.
 */

#define SBI_EXT_BASE 0x10
#define SBI_EXT_TIME 0x54494D45
#define SBI_EXT_IPI 0x735049
#define SBI_EXT_RFNC 0x52464E43
#define SBI_EXT_HSM  0x48534D
#define SBI_EXT_PMU  0x504D55
#define SBI_EXT_CONSOLE_PUTCHAR 0x01
#define SBI_EXT_CONSOLE_GETCHAR 0x02
#define SBI_EXT_DBCN 0x4442434E // Debug Console Extension
#define SBI_EXT_SRST 0x53525354 // System Reset Extension

// SBI Return Structure
typedef struct {
  long error;
  long value;
} sbi_ret_t;

// Inline assembly to trigger an `ecall` to the RISC-V Machine Mode (M-Mode)
// firmware
static inline sbi_ret_t sbi_call(long ext, long fid, long arg0, long arg1,
                                 long arg2, long arg3, long arg4, long arg5) {
  register long a0 asm("a0") = arg0;
  register long a1 asm("a1") = arg1;
  register long a2 asm("a2") = arg2;
  register long a3 asm("a3") = arg3;
  register long a4 asm("a4") = arg4;
  register long a5 asm("a5") = arg5;
  register long a6 asm("a6") = fid;
  register long a7 asm("a7") = ext;

  asm volatile("ecall"
               : "+r"(a0), "+r"(a1)
               : "r"(a0), "r"(a1), "r"(a2), "r"(a3), "r"(a4), "r"(a5), "r"(a6), "r"(a7)
               : "memory");

  sbi_ret_t ret = {a0, a1};
  return ret;
}

static inline sbi_ret_t sbi_call_simple(long ext, long fid, long arg0, long arg1, long arg2) {
    return sbi_call(ext, fid, arg0, arg1, arg2, 0, 0, 0);
}

// Console I/O wrappers
static inline void sbi_console_putchar(int ch) {
  sbi_call_simple(SBI_EXT_CONSOLE_PUTCHAR, 0, ch, 0, 0);
}

static inline int sbi_console_getchar(void) {
  sbi_ret_t ret = sbi_call_simple(SBI_EXT_CONSOLE_GETCHAR, 0, 0, 0, 0);
  return ret.error;
}

// DBCN (Debug Console) wrappers
static inline int sbi_dbcn_write_byte(uint8_t ch) {
  sbi_ret_t ret = sbi_call_simple(SBI_EXT_DBCN, 0, (long)ch, 0, 0);
  return (int)ret.error;
}

// Power Management wrapper (System Reset / Shutdown)
static inline void sbi_system_reset(uint32_t reset_type,
                                    uint32_t reset_reason) {
  sbi_call_simple(SBI_EXT_SRST, 0, reset_type, reset_reason, 0);
}

// Request the firmware to set the next timer interrupt
static inline void sbi_set_timer(uint64_t stime_value) {
  sbi_call_simple(SBI_EXT_TIME, 0, stime_value, 0, 0);
}

// Send an Inter-Processor Interrupt (IPI) to wake up other CPU cores
static inline void sbi_send_ipi(unsigned long hart_mask, unsigned long hart_mask_base) {
  sbi_call_simple(SBI_EXT_IPI, 0, hart_mask, hart_mask_base, 0);
}

// Send an IPI with payload (Assuming a custom SBI extension or future standard)
#define SBI_EXT_IPI_PAYLOAD 0x73504A

static inline void sbi_send_ipi_payload(const unsigned long *hart_mask,
                                        uint64_t payload) {
  sbi_call_simple(SBI_EXT_IPI_PAYLOAD, 0, (long)hart_mask, (long)payload, 0);
}

// SBI HSM (Hart State Management)
static inline int sbi_hart_start(unsigned long hartid, unsigned long start_addr, unsigned long opaque) {
    sbi_ret_t ret = sbi_call_simple(SBI_EXT_HSM, 0, hartid, start_addr, opaque);
    return ret.error;
}

static inline int sbi_hart_stop(void) {
    sbi_ret_t ret = sbi_call_simple(SBI_EXT_HSM, 1, 0, 0, 0);
    return ret.error;
}

static inline int sbi_hart_get_status(unsigned long hartid) {
    sbi_ret_t ret = sbi_call_simple(SBI_EXT_HSM, 2, hartid, 0, 0);
    return ret.error ? ret.error : ret.value;
}

// Kernel Entry point launched by OpenSBI
void kernel_main(uint64_t hartid, uint64_t device_tree_ptr);

#endif // BHARAT_SBI_H
