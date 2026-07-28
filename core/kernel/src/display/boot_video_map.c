#include "boot/boot_info.h"
#include "boot/boot_validate.h"
#include "kernel.h"
#include "hal/hal_pt.h"
#include "hal/hal_mpa.h"
#include "hal/hal_tlb.h"
#include "bharat/display/boot_video.h"
#include "bharat/display/display_caps.h"
#include "mm/physmap.h"

// Early boot video map logic.
int boot_video_map(const boot_info_t *boot) {
    if (!boot || boot->console.type != BOOT_CONSOLE_FRAMEBUFFER) return -1;

    // Strict validation check
    boot_validation_report_t report;
    if (boot_validate_console((boot_info_t*)boot, &report) != BOOT_OK) {
        return -1;
    }

    phys_addr_t fb_phys = boot->console.fb_phys_base;
    size_t fb_size = (size_t)boot->console.fb_height * boot->console.fb_pitch;

    /*
     * Resolve the kernel virtual address for the framebuffer MMIO region.
     *
     * On x86_64, boot.S establishes a 4 GiB identity map at BOTH the low half
     * (PML4[0]) and the high-half alias (PML4[256], base 0xFFFF800000000000).
     * physmap_phys_to_virt() returns (phys + 0xFFFF800000000000), which is
     * already backed by those boot-time 2 MiB huge pages — no new PT entries
     * are needed.  Calling hal_pt_map_range here would split the existing 2 MiB
     * page, producing an unreliable intermediate state and serving no purpose.
     *
     * For other architectures (ARM64, RISC-V) physmap_phys_to_virt() similarly
     * returns the correctly-mapped high-half address once the MMU is up.
     *
     * We only fall back to hal_pt_map_range if the physmap linear map is not
     * available (e.g. very early, or MPU-only targets).
     */
    virt_addr_t fb_virt = 0;

#if defined(__x86_64__)
    if (physmap_has_linear_map()) {
        void *virt_ptr = physmap_phys_to_virt(fb_phys);
        if (!virt_ptr) return -1;
        fb_virt = (virt_addr_t)(uintptr_t)virt_ptr;
    } else {
        /* No linear physmap — install an explicit mapping. */
        fb_virt = 0xFFFF800000000000ULL | (fb_phys & 0xFFFFFFFF);
        uint32_t flags = HAL_PT_FLAG_READ | HAL_PT_FLAG_WRITE | HAL_PT_FLAG_DEVICE;
        phys_addr_t current_root = active_mem_protect->cpu_ops.get_root();
        if (hal_pt_map_range(current_root, fb_virt, fb_phys, fb_size, flags) != 0) {
            return -1;
        }
        hal_tlb_invalidate_all();
    }
#else
    // For other architectures (ARM64, RISC-V), resolve virtual address via physmap.
    void *vptr = physmap_phys_to_virt(fb_phys);
    if (!vptr) return -1;
    fb_virt = (virt_addr_t)(uintptr_t)vptr;

    if (physmap_has_linear_map()) {
        phys_addr_t page_offset = fb_phys & 0xFFFU;
        phys_addr_t aligned_phys = fb_phys & ~0xFFFU;
        virt_addr_t aligned_virt = fb_virt & ~0xFFFU;
        size_t aligned_size = (fb_size + page_offset + 0xFFFU) & ~0xFFFU;

        phys_addr_t current_root = active_mem_protect ? active_mem_protect->cpu_ops.get_root() : 0U;
        if (current_root == 0) {
            extern phys_addr_t vmm_get_kernel_root(void);
            current_root = vmm_get_kernel_root();
        }

        phys_addr_t existing_pa = 0;
        size_t mapped_size = 0;
        uint32_t existing_flags = 0;
        bool already_mapped = false;

        if (current_root != 0 && hal_pt_query_mapping(current_root, aligned_virt, &existing_pa, &mapped_size, &existing_flags) == 0) {
            if (existing_pa == aligned_phys) {
                already_mapped = true;
            }
        }

        if (!already_mapped && current_root != 0) {
            uint32_t flags = HAL_PT_FLAG_READ | HAL_PT_FLAG_WRITE | HAL_PT_FLAG_DEVICE;
            if (hal_pt_map_range(current_root, aligned_virt, aligned_phys, aligned_size, flags) != 0) {
                return -1;
            }
            hal_tlb_invalidate_all();
        }
    }
#endif

    // Pre-populate the GUI handoff with the correct mapped virtual address so
    // that boot_gui_run() finds it valid and skips the boot_video_collect()
    // call that would otherwise overwrite virt_addr back to the physical address,
    // causing a page fault when the GUI first writes to the framebuffer.
#if BHARAT_BOOT_GUI
    extern boot_video_handoff_t* boot_video_get_handoff_ptr(void) __attribute__((weak));
    extern int boot_video_collect(boot_video_handoff_t *out);
    if (boot_video_get_handoff_ptr) {
        boot_video_handoff_t* handoff = boot_video_get_handoff_ptr();
        if (handoff && !handoff->valid) {
            // Fully populate the handoff from the machine layer now, while we
            // know the correct virtual address.  This prevents boot_gui_run()
            // from calling boot_video_collect() later (which would reset
            // virt_addr to the raw physical address before the mapping takes
            // effect in the GUI's framebuffer pointer).
            boot_video_collect(handoff);
        }
        if (handoff) {
            // Always stamp the remapped virtual address — even if the handoff
            // was already populated by a prior call, the mapping we just
            // resolved is what the CPU will actually use.
            handoff->virt_addr = fb_virt;
        }
    }
#endif

    return 0;
}

int qemu_display_map_mmio(uint64_t phys, size_t size, uintptr_t *out_virt) {
    if (phys == 0 || size == 0 || !out_virt) {
        return -1;
    }

    if (!physmap_has_linear_map()) {
        *out_virt = (uintptr_t)phys;
        return 0;
    }

    uint64_t page_offset = phys & 0xFFFU;
    uint64_t aligned_phys = phys & ~0xFFFU;
    uint64_t aligned_size = (size + page_offset + 0xFFFU) & ~0xFFFU;

    void *virt_ptr = physmap_phys_to_virt(phys);
    if (!virt_ptr) {
        return -1;
    }

    uintptr_t base_virt = (uintptr_t)virt_ptr;
    virt_addr_t aligned_virt = (virt_addr_t)base_virt & ~0xFFFU;

    if (active_mem_protect && active_mem_protect->cpu_ops.get_root) {
        phys_addr_t current_root = active_mem_protect->cpu_ops.get_root();
        if (current_root == 0) {
            extern phys_addr_t vmm_get_kernel_root(void);
            current_root = vmm_get_kernel_root();
        }
        if (current_root != 0) {
            phys_addr_t existing_pa = 0;
            size_t mapped_size = 0;
            uint32_t existing_flags = 0;
            if (hal_pt_query_mapping(current_root, aligned_virt, &existing_pa, &mapped_size, &existing_flags) != 0 ||
                existing_pa != aligned_phys) {
                uint32_t flags = HAL_PT_FLAG_READ | HAL_PT_FLAG_WRITE | HAL_PT_FLAG_DEVICE;
                (void)hal_pt_map_range(current_root, aligned_virt, aligned_phys, aligned_size, flags);
                hal_tlb_invalidate_all();
            }
        }
    }

    *out_virt = base_virt;
    return 0;
}
