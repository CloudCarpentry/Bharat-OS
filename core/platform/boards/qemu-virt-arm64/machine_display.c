/*
 * machine_display.c — QEMU virt ARM64 display capabilities
 *
 * QEMU virt (ARM64) can expose a VirtIO-GPU or a pl011 PL080 framebuffer.
 * With -device ramfb, we get a simple linear framebuffer that we can use
 * for the boot GUI without complex VirtIO negotiation.
 */
#include "bharat/display/display_caps.h"
#include "hal/hal_discovery.h"
#include "hal/hal_pt.h"
#include "hal/hal_tlb.h"
#include "hal/hal_mpa.h"
#include "mm/physmap.h"
#include <stdbool.h>
#include <stdint.h>

static inline const system_discovery_t *machine_discovery_ptr(void) {
    const system_discovery_t *raw = hal_get_system_discovery();
    if (!raw) {
        return NULL;
    }

#if defined(__aarch64__)
    /*
     * Defensive canonicalization for early-boot callers: if the pointer carries
     * a top-byte tag while TBI is not enabled yet, strip it before dereference.
     */
    uintptr_t untagged = ((uintptr_t)raw) & UINT64_C(0x00ffffffffffffff);
    return (const system_discovery_t *)untagged;
#else
    return raw;
#endif
}

static bool machine_discovery_boot_video(const boot_video_handoff_t **video_out) {
    if (video_out) {
        *video_out = NULL;
    }

    const system_discovery_t *discovery = machine_discovery_ptr();
    if (!discovery) {
        return false;
    }

    const boot_video_handoff_t *video = &discovery->boot_video;
    if (!video->valid) {
        return false;
    }

    if (video->width == 0 || video->height == 0 || video->stride_bytes == 0) {
        return false;
    }

    uint64_t required = (uint64_t)video->height * (uint64_t)video->stride_bytes;
    if (video->phys_addr == 0 || video->size < required) {
        return false;
    }

    if (video_out) {
        *video_out = video;
    }

    return true;
}

extern void hal_serial_write(const char *str);

static void log_hex32(uint32_t val) {
    char buf[11];
    buf[0] = '0'; buf[1] = 'x';
    const char *hex = "0123456789abcdef";
    for (int i = 7; i >= 0; i--) {
        buf[2 + i] = hex[val & 0xF];
        val >>= 4;
    }
    buf[10] = '\0';
    hal_serial_write(buf);
}

static void virt_scan_pci_ecam_for_vga(system_discovery_t *discovery) {
    if (!discovery || discovery->boot_video.valid) return;
    
    uintptr_t ecam_bases[5];
    size_t count = 0;
    
    if (discovery->pci_host_count > 0 && discovery->pci_hosts[0].ecam_base != 0) {
        ecam_bases[count++] = (uintptr_t)discovery->pci_hosts[0].ecam_base;
    }
    ecam_bases[count++] = 0x40100000ULL;
    ecam_bases[count++] = 0x30000000ULL;
    ecam_bases[count++] = 0x3F000000ULL;
    ecam_bases[count++] = 0x20000000ULL;
    
    hal_serial_write("[PCI Scan] Starting PCIe ECAM probe...\n");

    for (size_t b = 0; b < count; b++) {
        uintptr_t phys_ecam = ecam_bases[b];
        if (phys_ecam == 0) continue;
        
        uintptr_t virt_ecam = 0;
        if (physmap_has_linear_map()) {
            void *v = physmap_phys_to_virt(phys_ecam);
            if (v) virt_ecam = (uintptr_t)v;
        }
        if (virt_ecam == 0) virt_ecam = phys_ecam;
        
        for (int bus = 0; bus < 4; bus++) {
            for (int dev = 0; dev < 32; dev++) {
                volatile uint32_t *ecam_dev = (volatile uint32_t *)(virt_ecam + ((uintptr_t)bus << 20) + ((uintptr_t)dev << 15));
                uint32_t vendor_device = ecam_dev[0];
                
                if (vendor_device != 0xFFFFFFFF && vendor_device != 0x00000000) {
                    hal_serial_write("  [PCI Dev Found] ECAM Base: ");
                    log_hex32((uint32_t)phys_ecam);
                    hal_serial_write(" Bus: "); log_hex32(bus);
                    hal_serial_write(" Dev: "); log_hex32(dev);
                    hal_serial_write(" ID: "); log_hex32(vendor_device);
                    hal_serial_write("\n");

                    uint16_t vendor_id = vendor_device & 0xFFFF;
                    uint16_t device_id = (vendor_device >> 16) & 0xFFFF;

                    // Match Bochs VGA (0x1234:0x1111 or 0x1234:0x1110), QEMU stdvga (0x1234:0x0001), or RedHat VGA (0x1b36:0x0100 / 0x1b36:0x000d)
                    if (vendor_id == 0x1234 || (vendor_id == 0x1b36 && (device_id == 0x0100 || device_id == 0x000d))) {
                        hal_serial_write("  [VGA Match] Recognized Bochs/QEMU display device!\n");
                        uint32_t bar0 = ecam_dev[4]; // BAR0 (offset 0x10)
                        uint32_t bar2 = ecam_dev[6]; // BAR2 (offset 0x18)

                        // Assign BARs in QEMU virt PCIe 32-bit MMIO space (0x40000000+)
                        if ((bar0 & 0xFFFFFFF0) == 0) {
                            bar0 = 0x40000000 + ((uint32_t)dev * 0x1000000);
                            ecam_dev[4] = bar0;
                        }
                        if ((bar2 & 0xFFFFFFF0) == 0) {
                            bar2 = bar0 + 0x800000; // 8MB offset for MMIO control regs
                            ecam_dev[6] = bar2;
                        }
                        
                        // Enable Memory Space (bit 1) + Bus Master (bit 2) in Command Register
                        ecam_dev[1] |= 0x06;
                        
                        uint32_t fb_phys = bar0 & 0xFFFFFFF0;
                        uint32_t mmio_phys = bar2 & 0xFFFFFFF0;
                        
                        uintptr_t virt_mmio = 0;
                        if (physmap_has_linear_map()) {
                            void *v = physmap_phys_to_virt(mmio_phys);
                            if (v) virt_mmio = (uintptr_t)v;
                        }
                        if (virt_mmio == 0) virt_mmio = mmio_phys;

                        /* Ensure MMIO region is mapped in page tables if MMU is active */
                        if (active_mem_protect && active_mem_protect->cpu_ops.get_root) {
                            phys_addr_t root = active_mem_protect->cpu_ops.get_root();
                            if (root != 0) {
                                uint32_t map_flags = HAL_PT_FLAG_READ | HAL_PT_FLAG_WRITE | HAL_PT_FLAG_DEVICE;
                                hal_pt_map_range(root, virt_mmio, mmio_phys, 0x100000, map_flags);
                                hal_tlb_invalidate_all();
                            }
                        }
                        
                        /* Program Bochs VBE MMIO registers */
                        if (virt_mmio != 0) {
                            volatile uint16_t *vbe = (volatile uint16_t *)virt_mmio;
                            vbe[4] = 0x00; // VBE_DISPI_INDEX_ENABLE = 0
                            vbe[1] = 1024; // VBE_DISPI_INDEX_XRES = 1024
                            vbe[2] = 768;  // VBE_DISPI_INDEX_YRES = 768
                            vbe[3] = 32;   // VBE_DISPI_INDEX_BPP = 32
                            vbe[4] = 0x41; // VBE_DISPI_ENABLED | VBE_DISPI_LFB_ENABLED
                        }
                        
                        discovery->boot_video.phys_addr    = fb_phys;
                        discovery->boot_video.virt_addr    = fb_phys;
                        discovery->boot_video.width        = 1024;
                        discovery->boot_video.height       = 768;
                        discovery->boot_video.stride_bytes = 1024 * 4;
                        discovery->boot_video.size         = 1024 * 768 * 4;
                        discovery->boot_video.format       = PIXEL_FORMAT_ARGB8888;
                        discovery->boot_video.valid        = true;

                        /* Synchronize g_boot_info console for kernel_boot.c UI activation */
                        extern const boot_info_t *g_boot_info;
                        if (g_boot_info) {
                            boot_info_t *b = (boot_info_t *)g_boot_info;
                            b->console.type = BOOT_CONSOLE_FRAMEBUFFER;
                            b->console.fb_phys_base = fb_phys;
                            b->console.fb_width = 1024;
                            b->console.fb_height = 768;
                            b->console.fb_pitch = 1024 * 4;
                            b->console.fb_bpp = 32;
                        }
                        return;
                    }
                }
            }
        }
    }
}

int machine_get_display_caps(machine_display_caps_t *out) {
    if (!out) return -1;

    const system_discovery_t *raw_disc = hal_get_system_discovery();
    if (raw_disc) {
        virt_scan_pci_ecam_for_vga((system_discovery_t*)raw_disc);
    }

    const boot_video_handoff_t *video = NULL;
    if (machine_discovery_boot_video(&video)) {
        out->display_present           = true;
        out->boot_gui_allowed          = true;
        out->firmware_fb_possible      = true;
        out->early_simplefb_possible   = true;
        out->early_panel_init_possible = false;
        out->needs_gpu_firmware        = false;
        out->needs_complex_modeset     = false;
        out->input_present             = false;

        out->mmio_base  = video->phys_addr;
        out->mmio_size  = video->size;
        out->irq        = -1;
        out->max_width  = video->width;
        out->max_height = video->height;
    } else {
        out->display_present           = true;
        out->boot_gui_allowed          = false; /* defer to userspace */
        out->firmware_fb_possible      = false;
        out->early_simplefb_possible   = false;
        out->early_panel_init_possible = false;
        out->needs_gpu_firmware        = true;  /* VirtIO-GPU needs negotiation */
        out->needs_complex_modeset     = true;  /* modeset deferred */
        out->input_present             = false;

        out->mmio_base  = 0;
        out->mmio_size  = 0;
        out->irq        = -1;
        out->max_width  = 1920;
        out->max_height = 1080;
    }

    return 0;
}

int machine_probe_boot_video(display_probe_result_t *out,
                             boot_video_handoff_t   *video) {
    if (!out) return -1;

    system_discovery_t* discovery = hal_get_system_discovery();
    if (discovery) {
        virt_scan_pci_ecam_for_vga(discovery);
    }

    const boot_video_handoff_t *discovered = NULL;
    if (machine_discovery_boot_video(&discovered)) {
        out->usable            = true;
        out->path              = BOOT_VIDEO_PATH_FIRMWARE_FB;
        out->quality_score     = 90;
        out->early_usable      = true;
        out->interactive       = false;
        out->requires_takeover = true;

        if (video) {
            *video = *discovered;
        }
    } else {
        out->usable            = false;
        out->path              = BOOT_VIDEO_PATH_VIRTIO_GPU_LATE;
        out->quality_score     = 0;
        out->early_usable      = false;
        out->interactive       = false;
        out->requires_takeover = true;

        if (video) {
            video->valid = false;
        }
    }

    return 0;
}
