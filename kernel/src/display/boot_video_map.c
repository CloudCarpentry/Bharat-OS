#include "boot/boot_info.h"
#include "boot/boot_validate.h"
#include "kernel.h"
#include "hal/hal_pt.h"
#include "hal/hal_mpa.h"
#include "bharat/display/boot_video.h"

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
    uint32_t flags = HAL_PT_FLAG_READ | HAL_PT_FLAG_WRITE;

    // Use a high canonical address to avoid clashing with identity mapping.
    // Base: 0xFFFF900000000000
    virt_addr_t fb_virt = 0xFFFF900000000000ULL | (fb_phys & 0xFFFFFFFF);

    phys_addr_t current_root = active_mem_protect->cpu_ops.get_root();
    
    if (hal_pt_map_range(current_root, fb_virt, fb_phys, fb_size, flags) != 0) {
        return -1;
    }
    
    // Full TLB flush
    __asm__ volatile("mov %%cr3, %%rax; mov %%rax, %%cr3" ::: "rax", "memory");

    // Update the handoff's virtual address for the GUI to find.
    extern boot_video_handoff_t* boot_video_get_handoff_ptr(void);
    boot_video_handoff_t* handoff = boot_video_get_handoff_ptr();
    if (handoff) {
        handoff->virt_addr = fb_virt;
    }

    return 0;
}
