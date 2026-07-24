#include <bharat/cpu_local.h>

#define MSR_EFER         0xc0000080
#define MSR_STAR         0xc0000081
#define MSR_LSTAR        0xc0000082
#define MSR_FMASK        0xc0000084
#define MSR_GS_BASE      0xc0000101
#define MSR_KERNEL_GS_BASE 0xc0000102

#define EFER_SCE         0x00000001

extern void x86_64_syscall_entry(void);

void arch_cpu_local_set(cpu_local_t *cl) {
    if (!cl) return;

    cl->self = cl;

    uintptr_t addr = (uintptr_t)cl;
    uint32_t low = (uint32_t)addr;
    uint32_t high = (uint32_t)(addr >> 32);

    __asm__ volatile("wrmsr" : : "a"(low), "d"(high), "c"(MSR_GS_BASE));
    __asm__ volatile("wrmsr" : : "a"(low), "d"(high), "c"(MSR_KERNEL_GS_BASE));
}

void x86_64_init_syscall(void) {
    uint32_t eax, edx;

    // Enable SYSCALL/SYSRET
    __asm__ volatile("rdmsr" : "=a"(eax), "=d"(edx) : "c"(MSR_EFER));
    eax |= EFER_SCE;
    __asm__ volatile("wrmsr" : : "a"(eax), "d"(edx), "c"(MSR_EFER));

    // Configure STAR:
    // Bits 47:32 - Kernel CS (loads 0x08, SS=0x10)
    // Bits 63:48 - User CS base (loads CS=0x23, SS=0x1B)
    uint32_t star_low = 0;
    uint32_t star_high = (0x0008U << 0) | (0x0013U << 16); // 0x08 for Kernel, 0x13 for User (index 2 | 3)
    __asm__ volatile("wrmsr" : : "a"(star_low), "d"(star_high), "c"(MSR_STAR));

    // Configure LSTAR
    uintptr_t entry = (uintptr_t)x86_64_syscall_entry;
    __asm__ volatile("wrmsr" : : "a"((uint32_t)entry), "d"((uint32_t)(entry >> 32)), "c"(MSR_LSTAR));

    // Configure FMASK: mask IF (9), TF (8), DF (10), AC (18), NT (14)
    uint32_t fmask = 0x44700;
    __asm__ volatile("wrmsr" : : "a"(fmask), "d"(0), "c"(MSR_FMASK));
}
