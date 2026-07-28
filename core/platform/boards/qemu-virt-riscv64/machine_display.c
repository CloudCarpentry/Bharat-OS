/*
 * machine_display.c — QEMU virt RISC-V display capabilities
 *
 * QEMU virt (RISC-V) has no native framebuffer at early boot.
 * VirtIO-GPU is available but requires a full driver (deferred to
 * userspace or a later boot stage).  We therefore report no display
 * and let the boot UI resolver fall gracefully back to text mode.
 *
 * The weak fallback in display_fallback.c already handles this, but
 * an explicit implementation here makes the board intent clear and
 * suppresses the "using weak symbol" behaviour which can mask bugs.
 */
#include "bharat/display/display_caps.h"
#include "hal/hal_discovery.h"
#include "hal/hal_pt.h"
#include "hal/hal_tlb.h"
#include "hal/hal_mpa.h"
#include "mm/physmap.h"
#include <stdbool.h>

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

static void log_hex64(uint64_t val) {
    hal_serial_write("0x");
    const char *hex = "0123456789abcdef";
    char buf[17];
    for (int i = 15; i >= 0; i--) {
        buf[i] = hex[val & 0xF];
        val >>= 4;
    }
    buf[16] = '\0';
    hal_serial_write(buf);
}

static void virt_scan_pci_ecam_for_vga(system_discovery_t *discovery) {
    if (!discovery || discovery->boot_video.valid) return;
    
    if (discovery->pci_host_count == 0 || discovery->pci_hosts[0].ecam_base == 0) {
        hal_serial_write("BHARAT_DISPLAY:FAIL=NO_PCI_HOST\n");
        return;
    }

    uint64_t phys_ecam = discovery->pci_hosts[0].ecam_base;
    uint64_t ecam_size = discovery->pci_hosts[0].ecam_size;
    if (ecam_size == 0) ecam_size = 0x1000000;

    hal_serial_write("BHARAT_DISPLAY:PCI_ECAM=");
    log_hex64(phys_ecam);
    hal_serial_write("\n");

    uintptr_t virt_ecam = 0;
    if (qemu_display_map_mmio(phys_ecam, ecam_size, &virt_ecam) != 0) {
        hal_serial_write("BHARAT_DISPLAY:FAIL=ECAM_MAP\n");
        return;
    }
    hal_serial_write("BHARAT_DISPLAY:ECAM_MAP=PASS\n");

    uint64_t mmio_base = discovery->pci_hosts[0].mmio32_base;
    uint64_t mmio_size = discovery->pci_hosts[0].mmio32_size;
    if (mmio_base == 0 || mmio_size == 0) {
        // Fallback for RISC-V 64 QEMU virt PCIe MMIO range
        mmio_base = 0x40000000ULL;
        mmio_size = 0x20000000ULL;
    }

    hal_serial_write("BHARAT_DISPLAY:PCI_MMIO_BASE=");
    log_hex64(mmio_base);
    hal_serial_write("\n");
    hal_serial_write("BHARAT_DISPLAY:PCI_MMIO_SIZE=");
    log_hex64(mmio_size);
    hal_serial_write("\n");

    bool vga_found = false;

    for (int bus = 0; bus < 4 && !vga_found; bus++) {
        for (int dev = 0; dev < 32 && !vga_found; dev++) {
            volatile uint32_t *ecam_dev = (volatile uint32_t *)(virt_ecam + ((uintptr_t)bus << 20) + ((uintptr_t)dev << 15));
            uint32_t vendor_device = ecam_dev[0];

            if (vendor_device != 0xFFFFFFFF && vendor_device != 0x00000000) {
                uint16_t vendor_id = vendor_device & 0xFFFF;
                uint16_t device_id = (vendor_device >> 16) & 0xFFFF;

                if (vendor_id == 0x1234 || (vendor_id == 0x1b36 && (device_id == 0x0100 || device_id == 0x000d))) {
                    hal_serial_write("BHARAT_DISPLAY:VGA_FOUND\n");
                    vga_found = true;

                    // Read current BARs
                    uint32_t orig_bar0 = ecam_dev[4];
                    uint32_t orig_bar2 = ecam_dev[6];

                    // Probe sizes
                    ecam_dev[4] = 0xFFFFFFFF;
                    uint32_t mask0 = ecam_dev[4];
                    ecam_dev[4] = orig_bar0;

                    ecam_dev[6] = 0xFFFFFFFF;
                    uint32_t mask2 = ecam_dev[6];
                    ecam_dev[6] = orig_bar2;

                    uint32_t size0 = ~(mask0 & 0xFFFFFFF0) + 1;
                    uint32_t size2 = ~(mask2 & 0xFFFFFFF0) + 1;

                    if (size0 == 0 || size0 > 0x10000000) size0 = 0x1000000; // default 16MB
                    if (size2 == 0 || size2 > 0x10000000) size2 = 0x1000;    // default 4KB

                    uint32_t bar0 = orig_bar0;
                    uint32_t bar2 = orig_bar2;

                    // Assign BAR0 if unprogrammed
                    if ((bar0 & 0xFFFFFFF0) == 0) {
                        bar0 = (uint32_t)mmio_base;
                        ecam_dev[4] = bar0;
                    }
                    // Assign BAR2 if unprogrammed
                    if ((bar2 & 0xFFFFFFF0) == 0) {
                        uint32_t aligned_bar2 = (bar0 + size0 + size2 - 1) & ~(size2 - 1);
                        bar2 = aligned_bar2;
                        ecam_dev[6] = bar2;
                    }

                    // Enable Memory Space (bit 1) + Bus Master (bit 2)
                    ecam_dev[1] |= 0x06;

                    uint64_t fb_phys = bar0 & 0xFFFFFFF0;
                    uint64_t mmio_phys = bar2 & 0xFFFFFFF0;

                    hal_serial_write("BHARAT_DISPLAY:BAR0=");
                    log_hex64(fb_phys);
                    hal_serial_write("\n");
                    hal_serial_write("BHARAT_DISPLAY:BAR2=");
                    log_hex64(mmio_phys);
                    hal_serial_write("\n");

                    // Validate BAR allocations are within discovered PCI MMIO boundaries and don't overlap with RAM
                    if (fb_phys < mmio_base || fb_phys + size0 > mmio_base + mmio_size ||
                        mmio_phys < mmio_base || mmio_phys + size2 > mmio_base + mmio_size) {
                        hal_serial_write("BHARAT_DISPLAY:FAIL=BAR_ALLOC_OUT_OF_BOUNDS\n");
                        return;
                    }

                    // On RISC-V 64, RAM starts at 0x80000000. Ensure no overlap with RAM range!
                    if ((fb_phys >= 0x80000000 && fb_phys < 0x80000000 + 0x40000000) ||
                        (mmio_phys >= 0x80000000 && mmio_phys < 0x80000000 + 0x40000000)) {
                        hal_serial_write("BHARAT_DISPLAY:FAIL=BAR_ALLOC_RAM_OVERLAP\n");
                        return;
                    }

                    // Map control BAR
                    uintptr_t virt_mmio = 0;
                    if (qemu_display_map_mmio(mmio_phys, size2, &virt_mmio) != 0) {
                        hal_serial_write("BHARAT_DISPLAY:FAIL=CTRL_MAP\n");
                        return;
                    }
                    hal_serial_write("BHARAT_DISPLAY:CTRL_MAP=PASS\n");

                    // Program Bochs VBE registers
                    if (virt_mmio != 0) {
                        volatile uint16_t *vbe = (volatile uint16_t *)virt_mmio;
                        vbe[4] = 0x00; // VBE_DISPI_INDEX_ENABLE = 0
                        vbe[1] = 1024; // VBE_DISPI_INDEX_XRES = 1024
                        vbe[2] = 768;  // VBE_DISPI_INDEX_YRES = 768
                        vbe[3] = 32;   // VBE_DISPI_INDEX_BPP = 32
                        vbe[4] = 0x41; // VBE_DISPI_ENABLED | VBE_DISPI_LFB_ENABLED
                    }

                    // Map Framebuffer BAR
                    uintptr_t virt_fb = 0;
                    if (qemu_display_map_mmio(fb_phys, size0, &virt_fb) != 0) {
                        hal_serial_write("BHARAT_DISPLAY:FAIL=FB_MAP\n");
                        return;
                    }
                    hal_serial_write("BHARAT_DISPLAY:FB_MAP=PASS\n");

                    discovery->boot_video.phys_addr    = fb_phys;
                    discovery->boot_video.virt_addr    = virt_fb;
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

                    hal_serial_write("BHARAT_DISPLAY:MODE=1024x768x32\n");
                    hal_serial_write("BHARAT_DISPLAY:READY\n");
                    return;
                }
            }
        }
    }

    if (!vga_found) {
        hal_serial_write("BHARAT_DISPLAY:FAIL=VGA_NOT_FOUND\n");
    }
}

int machine_get_display_caps(machine_display_caps_t *out) {
    if (!out) return -1;

    system_discovery_t* discovery = hal_get_system_discovery();
    if (discovery) {
        virt_scan_pci_ecam_for_vga(discovery);
    }
    
    if (discovery && discovery->boot_video.valid) {
        out->display_present         = true;
        out->boot_gui_allowed        = true;
        out->firmware_fb_possible    = true;
        out->early_simplefb_possible = true;
        out->early_panel_init_possible = false;
        out->needs_gpu_firmware      = false;
        out->needs_complex_modeset   = false;
        out->input_present           = false;

        out->mmio_base   = discovery->boot_video.phys_addr;
        out->mmio_size   = discovery->boot_video.size;
        out->irq         = -1;
        out->max_width   = discovery->boot_video.width;
        out->max_height  = discovery->boot_video.height;
    } else {
        out->display_present         = false;
        out->boot_gui_allowed        = false;
        out->firmware_fb_possible    = false;
        out->early_simplefb_possible = false;
        out->early_panel_init_possible = false;
        out->needs_gpu_firmware      = false;
        out->needs_complex_modeset   = false;
        out->input_present           = false;
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

    if (discovery && discovery->boot_video.valid) {
        out->usable          = true;
        out->path            = BOOT_VIDEO_PATH_FIRMWARE_FB;
        out->quality_score   = 90;
        out->early_usable    = true;
        out->interactive     = false;
        out->requires_takeover = true;

        if (video) {
            *video = discovery->boot_video;
        }
    } else {
        out->usable          = false;
        out->path            = BOOT_VIDEO_PATH_NONE;
        out->quality_score   = 0;
        out->early_usable    = false;
        out->interactive     = false;
        out->requires_takeover = false;

        if (video) {
            video->valid = false;
        }
    }

    return 0;
}
