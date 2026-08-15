#include "hal/hal_boot.h"
#include "hal/hal_discovery.h"
#include "console/console_core.h"
#include <stdint.h>

#define KPRINT(s) console_write_raw(s, string_length(s))

// PSCI Function IDs
#define PSCI_VERSION            0x84000000
#define PSCI_CPU_ON_64          0xC4000003

// PSCI Return Error Codes
#define PSCI_SUCCESS             0
#define PSCI_NOT_SUPPORTED      -1
#define PSCI_INVALID_PARAMS     -2
#define PSCI_DENIED             -3
#define PSCI_ALREADY_ON         -4
#define PSCI_ON_PENDING         -5
#define PSCI_INTERNAL_FAILURE   -6
#define PSCI_NOT_PRESENT        -7
#define PSCI_DISABLED           -8
#define PSCI_INVALID_ADDRESS    -9

static int64_t psci_call(uint32_t method, uint64_t fn_id, uint64_t arg0, uint64_t arg1, uint64_t arg2) {
    register uint64_t r0 asm("x0") = fn_id;
    register uint64_t r1 asm("x1") = arg0;
    register uint64_t r2 asm("x2") = arg1;
    register uint64_t r3 asm("x3") = arg2;

    if (method == 1) { // SMC
        __asm__ volatile(
            "smc #0"
            : "+r"(r0), "+r"(r1), "+r"(r2), "+r"(r3)
            :
            : "x4", "x5", "x6", "x7", "memory"
        );
    } else if (method == 2) { // HVC
        __asm__ volatile(
            "hvc #0"
            : "+r"(r0), "+r"(r1), "+r"(r2), "+r"(r3)
            :
            : "x4", "x5", "x6", "x7", "memory"
        );
    } else {
        return PSCI_NOT_SUPPORTED;
    }

    return (int64_t)r0;
}

void secondary_entry_arch_early(void) {
    // Setup initial architecture-specific state for secondary core (FPU/SIMD)
    uint64_t cpacr;
    __asm__ volatile("mrs %0, cpacr_el1" : "=r"(cpacr));
    cpacr |= (3ULL << 20); // FPEN = 0b11: no FP/SIMD traps
    __asm__ volatile("msr cpacr_el1, %0\n isb" :: "r"(cpacr));
}

void secondary_entry_arch_late(void) {
    // Enable local interrupts (PSTATE.I)
    __asm__ volatile("msr daifclr, #2"); // Clear interrupt mask (enable IRQs)
}

int hal_boot_start_cpu(uint32_t cpu_id, uint64_t entry_point) {
    system_discovery_t* disc = hal_get_system_discovery();
    if (!disc) {
        return -1;
    }

    uint32_t method = disc->psci_method;
    if (method == 0) {
        KPRINT("  [PSCI] Error: PSCI method not discovered; refusing guessed SMC/HVC.\n");
        return PSCI_NOT_PRESENT;
    }

    // Probe PSCI version once
    if (disc->psci_version == 0) {
        int64_t ver = psci_call(method, PSCI_VERSION, 0, 0, 0);
        if (ver < 0) {
            KPRINT("  [PSCI] Error: PSCI version probe failed.\n");
            return -1;
        }
        disc->psci_version = (uint32_t)ver;

        uint32_t major = (uint32_t)(ver >> 16) & 0xFFFF;
        uint32_t minor = (uint32_t)ver & 0xFFFF;
        KPRINT("  [PSCI] Discovered version ");
        char buf[8];
        buf[0] = '0' + (major % 10);
        buf[1] = '.';
        buf[2] = '0' + (minor % 10);
        buf[3] = '\n';
        buf[4] = '\0';
        console_write_raw(buf, 4);

        if (major == 0 && minor < 2) {
            KPRINT("  [PSCI] Error: Legacy PSCI 0.1 is not supported.\n");
            return -2;
        }
    }

    // Translate logical cpu_id to physical MPIDR affinity value
    uint64_t target_mpidr = cpu_id; // Fallback
    bool found = false;
    for (uint32_t i = 0; i < disc->topology.cpu_count; i++) {
        if (disc->topology.cpus[i].cpu_id == cpu_id) {
            target_mpidr = disc->topology.cpus[i].hw_id;
            found = true;
            break;
        }
    }

    if (!found) {
        KPRINT("  [PSCI] Warning: Logical CPU ID not found in topology mapping. Using direct logical ID.\n");
    }

    KPRINT("  [PSCI] Calling CPU_ON for MPIDR ");
    char m_buf[16];
    m_buf[0] = '0' + (target_mpidr % 10);
    m_buf[1] = '\n';
    m_buf[2] = '\0';
    console_write_raw(m_buf, 2);

    // Assembly trampoline entry point
    extern void _secondary_trampoline(void);
    uint64_t trampoline_phys = (uint64_t)(uintptr_t)_secondary_trampoline;

    // Call PSCI CPU_ON
    // Parameters: target_cpu (MPIDR), entry_point (physical trampoline address), context_id (physical context pointer)
    int64_t res = psci_call(method, PSCI_CPU_ON_64, target_mpidr, trampoline_phys, entry_point);

    if (res == PSCI_SUCCESS || res == PSCI_ALREADY_ON) {
        return 0;
    }

    KPRINT("  [PSCI] Error: CPU_ON failed with result ");
    char err_buf[16];
    err_buf[0] = '-';
    err_buf[1] = '0' + ((-res) % 10);
    err_buf[2] = '\n';
    err_buf[3] = '\0';
    console_write_raw(err_buf, 3);

    return (int)res;
}
