#include <bharat/cpu_local.h>

#define MSR_EFER         0xc0000080
#define MSR_STAR         0xc0000081
#define MSR_LSTAR        0xc0000082
#define MSR_FMASK        0xc0000084

#define EFER_SCE         0x00000001

extern void x86_64_syscall_entry(void);

void arch_cpu_local_set(cpu_local_t *cl) {
    // Usually written to MSR_GS_BASE by HAL
    // We defer to HAL or fallback array lookup.
    (void)cl;
}

void x86_64_init_syscall(void) {
    uint32_t eax, edx;

    // Enable SYSCALL/SYSRET
    __asm__ volatile("rdmsr" : "=a"(eax), "=d"(edx) : "c"(MSR_EFER));
    eax |= EFER_SCE;
    __asm__ volatile("wrmsr" : : "a"(eax), "d"(edx), "c"(MSR_EFER));

    // Configure STAR:
    // Bits 47:32 - Kernel CS (and SS is CS + 8)
    // Bits 63:48 - User CS (and SS is User CS + 8, but for 64-bit it's different)
    // For 64-bit:
    // Kernel: CS = base, SS = base + 8
    // User: CS = base + 16, SS = base + 8
    uint32_t star_low = 0;
    uint32_t star_high = (0x0008U << 0) | (0x001BU << 16); // Kernel CS=0x08, User CS=0x1B (0x18 | 3)
    __asm__ volatile("wrmsr" : : "a"(star_low), "d"(star_high), "c"(MSR_STAR));

    // Configure LSTAR
    uintptr_t entry = (uintptr_t)x86_64_syscall_entry;
    __asm__ volatile("wrmsr" : : "a"((uint32_t)entry), "d"((uint32_t)(entry >> 32)), "c"(MSR_LSTAR));

    // Configure FMASK: mask IF (9), TF (8), DF (10)
    uint32_t fmask = 0x700;
    __asm__ volatile("wrmsr" : : "a"(fmask), "d"(0), "c"(MSR_FMASK));
}
