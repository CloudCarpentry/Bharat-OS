#include "hal/hal_mpa.h"
#include "hal/mmu_ops.h"
#include "hal/hal_discovery.h"
#include "hal/hal.h"
#include "capability.h"
#include "mm/aspace.h"
#include "console/console_core.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

mmu_ops_t *active_mmu = NULL;

// TODO: Needs refactor: #include directive placed mid-file for dependency/order compatibility.
#include "profile/profile.h"

// Forward declarations for arch-specific ops
extern mmu_ops_t x86_64_mmu_ops;
extern mmu_ops_t arm64_mmu_ops;
extern mmu_ops_t riscv64_mmu_ops;

// Forward declarations for runtime detection
extern void x86_mmu_detect(mmu_ops_t *ops);
extern void arm64_mmu_detect(mmu_ops_t *ops);
extern void riscv_mmu_detect(mmu_ops_t *ops);

// Forward declarations for IOMMU detection
extern void x86_iommu_detect(void);
extern void arm64_iommu_detect(void);
extern void riscv_iommu_detect(void);

// TODO: Needs refactor: #include directive placed mid-file for dependency/order compatibility.
#include "mm/prot_domain.h"

// Initialize based on memory model profile
void arch_mmu_init(void) {
    MemoryModel model = get_memory_model();

    if (model == MEM_MODEL_MMU) {
#if defined(__x86_64__)
        active_mmu = &x86_64_mmu_ops;
        x86_mmu_detect(active_mmu);
        x86_iommu_detect();

#elif defined(__aarch64__)
        active_mmu = &arm64_mmu_ops;
        arm64_mmu_detect(active_mmu);
        arm64_iommu_detect();

#elif defined(__riscv) && __riscv_xlen == 64
        active_mmu = &riscv64_mmu_ops;
        riscv_mmu_detect(active_mmu);
        riscv_iommu_detect();
#else
        active_mmu = NULL;
#endif
    } else {
        // MPU or FLAT profile: Do not provide MMU ops.
        active_mmu = NULL;
    }
}

void hal_mmu_final_setup(void) {
    if (!active_mem_protect) return;
    extern address_space_t kernel_space;

#if defined(__aarch64__) || defined(__riscv)
    system_discovery_t* discovery = hal_get_system_discovery();
    extern const uint64_t g_kernel_virt_offset;
#if defined(__aarch64__)
    extern void arm64_set_kernel_linear_map_state(phys_addr_t phys_base, size_t size, bool enabled);
#endif
    if (discovery) {
        uint64_t min_base = UINT64_MAX;
        uint64_t max_end = 0;
        // Map RAM
        for (uint32_t i = 0; i < discovery->topology.mem_region_count; i++) {
            uint64_t base = discovery->topology.mem_regions[i].base;
            uint64_t size = discovery->topology.mem_regions[i].size;
            if (base < min_base) {
                min_base = base;
            }
            if (base + size > max_end) {
                max_end = base + size;
            }
            for (uint64_t off = 0; off < size; off += 4096) {
                // Identity map
                if (vmm_map_page(base + off, base + off,
                             MMU_READ | MMU_WRITE | MMU_EXEC) != 0) {
                     hal_serial_write("FAIL Identity map at ");
                     hal_serial_write_hex(base + off);
                     hal_serial_write("\n");
                }
                // Linear high-half map
                if (vmm_map_page(base + off + g_kernel_virt_offset, base + off,
                             MMU_READ | MMU_WRITE | MMU_EXEC) != 0) {
                     hal_serial_write("FAIL Linear map at ");
                     hal_serial_write_hex(base + off + g_kernel_virt_offset);
                     hal_serial_write("\n");
                }
            }
        }

        // Map UART (essential for KPRINT after MMU enable)
#if defined(__aarch64__)
        vmm_map_page(0x09000000, 0x09000000, MMU_READ | MMU_WRITE | MMU_DEVICE);
#elif defined(__riscv)
        vmm_map_page(0x10000000, 0x10000000, MMU_READ | MMU_WRITE | MMU_DEVICE);
#endif

        // Map IRQ Controllers
        hal_serial_write("MMU: Found IRQ Controllers: ");
        hal_serial_write_hex(discovery->irq_ctrl_count);
        hal_serial_write("\n");

        for (uint32_t i = 0; i < discovery->irq_ctrl_count; i++) {
            uint64_t base = discovery->irq_ctrls[i].base;
            uint64_t size = discovery->irq_ctrls[i].size;
            hal_serial_write("MMU: Mapping IRQ CTRL ");
            hal_serial_write_hex(i);
            hal_serial_write(" at ");
            hal_serial_write_hex(base);
            hal_serial_write("\n");

            if (base != 0 && size != 0) {
                for (uint64_t off = 0; off < size; off += 4096) {
                    vmm_map_page(base + off, base + off,
                                 MMU_READ | MMU_WRITE | MMU_DEVICE);
                }
            }
            uint64_t aux_base = discovery->irq_ctrls[i].aux_base;
            uint64_t aux_size = discovery->irq_ctrls[i].aux_size;
            if (aux_base != 0 && aux_size != 0) {
                for (uint64_t off = 0; off < aux_size; off += 4096) {
                    vmm_map_page(aux_base + off, aux_base + off,
                                 MMU_READ | MMU_WRITE | MMU_DEVICE);
                }
            }
        }

        // Map Timers
        for (uint32_t i = 0; i < discovery->timer_count; i++) {
            uint64_t base = discovery->timers[i].base;
            uint64_t size = discovery->timers[i].size;
            if (base != 0 && size != 0) {
                for (uint64_t off = 0; off < size; off += 4096) {
                    vmm_map_page(base + off, base + off,
                                 MMU_READ | MMU_WRITE | MMU_DEVICE);
                }
            }
        }

        // Map Framebuffer if valid
        if (discovery->boot_video.valid) {
            uint64_t fb_phys = discovery->boot_video.phys_addr;
            uint64_t fb_size = discovery->boot_video.size;
            for (uint64_t off = 0; off < fb_size; off += 4096) {
                vmm_map_page(fb_phys + off, fb_phys + off,
                             MMU_READ | MMU_WRITE | MMU_DEVICE);
            }
        }

        active_mem_protect->cpu_ops.set_root(kernel_space.root_pt);

#if defined(__aarch64__)
        if (min_base != UINT64_MAX && max_end > min_base) {
            arm64_set_kernel_linear_map_state(min_base, (size_t)(max_end - min_base), true);
        } else {
            arm64_set_kernel_linear_map_state(0, 0, false);
        }
#endif
        return;
    }
#elif defined(__x86_64__)
    extern bool g_x86_mmu_finalized;
    hal_serial_write("MMU: Mapping RAM (x86_64)...\n");

    // Map RAM into high-half physical map
    extern const uint64_t g_kernel_virt_offset;

    // Identity map lower 128MB for transition stability (reduced from 1GB for speed)
    for (uint64_t addr = 0; addr < 0x8000000; addr += 4096) {
        vmm_map_page(addr, addr, MMU_READ | MMU_WRITE | MMU_EXEC);
        if ((addr % 0x1000000) == 0) hal_serial_write("."); 
    }
    hal_serial_write("\n");

    // Map hardware ranges (APIC/IOAPIC) - identity mapping to avoid triple faults on transition
    vmm_map_page(0xFEE00000, 0xFEE00000, MMU_READ | MMU_WRITE | MMU_NOCACHE);
    hal_serial_write("H");
    vmm_map_page(0xFEC00000, 0xFEC00000, MMU_READ | MMU_WRITE | MMU_NOCACHE);
    hal_serial_write("I\n");

    system_discovery_t* discovery = hal_get_system_discovery();
    if (discovery) {
        for (uint32_t i = 0; i < discovery->topology.mem_region_count; i++) {
            uint64_t base = discovery->topology.mem_regions[i].base;
            uint64_t size = discovery->topology.mem_regions[i].size;

            if (discovery->topology.mem_regions[i].type == HAL_MEM_RAM) {
                for (uint64_t off = 0; off < size; off += 0x1000) {
                    // Linear high-half map
                    vmm_map_page(base + off + g_kernel_virt_offset, base + off,
                                 MMU_READ | MMU_WRITE | MMU_EXEC);
                    // Identity map as well for stability during transition and for early DMA/PCI access
                    vmm_map_page(base + off, base + off,
                                 MMU_READ | MMU_WRITE | MMU_EXEC);
                    
                    if ((off % 0x2000000) == 0) hal_serial_write("m");
                }
            }
        }
    }
#endif
#ifdef BHARAT_ARCH_X86
    extern void x86_64_init_hardening(void);
    x86_64_init_hardening();
#endif

    hal_serial_write("MMU: Activating Root...\n");
    active_mem_protect->cpu_ops.set_root(kernel_space.root_pt);
#ifdef BHARAT_ARCH_X86
    g_x86_mmu_finalized = true;
#endif
    hal_serial_write("MMU: Root Activated.\n");
}
