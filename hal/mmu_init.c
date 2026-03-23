#include "hal/mmu_ops.h"
#include "hal/hal_discovery.h"
#include "hal/hal_pt.h"
#include "mm/aspace.h"
#include "console/console_core.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

mmu_ops_t *active_mmu = NULL;

// TODO: Needs refactor: #include directive placed mid-file for dependency/order compatibility.
#include "../../include/profile.h"

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
    if (!active_mmu) return;
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
                vmm_map_page(base + off, base + off,
                             CAP_RIGHT_READ | CAP_RIGHT_WRITE | CAP_RIGHT_EXECUTE);
                // Linear high-half map
                vmm_map_page(base + off + g_kernel_virt_offset, base + off,
                             CAP_RIGHT_READ | CAP_RIGHT_WRITE);
            }
        }

        // Map UART (essential for KPRINT after MMU enable)
#if defined(__aarch64__)
        vmm_map_page(0x09000000, 0x09000000, CAP_RIGHT_READ | CAP_RIGHT_WRITE | CAP_RIGHT_DEVICE);
#elif defined(__riscv)
        vmm_map_page(0x10000000, 0x10000000, CAP_RIGHT_READ | CAP_RIGHT_WRITE | CAP_RIGHT_DEVICE);
#endif

        // Map Framebuffer if valid
        if (discovery->boot_video.valid) {
            uint64_t fb_phys = discovery->boot_video.phys_addr;
            uint64_t fb_size = discovery->boot_video.size;
            for (uint64_t off = 0; off < fb_size; off += 4096) {
                vmm_map_page(fb_phys + off, fb_phys + off,
                             CAP_RIGHT_READ | CAP_RIGHT_WRITE | CAP_RIGHT_DEVICE_GPU);
            }
        }

        active_mmu->activate(kernel_space.root_pt);

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
    vmm_map_page(0xFEE00000, 0xFEE00000, CAP_RIGHT_READ | CAP_RIGHT_WRITE); // LAPIC
    vmm_map_page(0xFEC00000, 0xFEC00000, CAP_RIGHT_READ | CAP_RIGHT_WRITE); // IOAPIC

    // Map RAM into high-half physical map
    extern const uint64_t g_kernel_virt_offset;
    system_discovery_t* discovery = hal_get_system_discovery();
    if (discovery) {
        for (uint32_t i = 0; i < discovery->topology.mem_region_count; i++) {
            uint64_t base = discovery->topology.mem_regions[i].base;
            uint64_t size = discovery->topology.mem_regions[i].size;
            // Map the whole region into the high-half physical map
            for (uint64_t off = 0; off < size; off += 4096) {
                vmm_map_page(base + off + g_kernel_virt_offset, base + off,
                             CAP_RIGHT_READ | CAP_RIGHT_WRITE);
            }
        }
    }
#endif
    active_mmu->activate(kernel_space.root_pt);
}
