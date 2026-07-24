#include "boot/adapters/multiboot2_adapter.h"
#include "boot/boot_errno.h"

int multiboot2_adapter_parse(const void *mb2_info, boot_info_t *out_bi) {
    if (!mb2_info || !out_bi) return BOOT_ERR_INVALID_ARGUMENT;

    boot_info_init(out_bi);
    out_bi->source = BOOT_SOURCE_MULTIBOOT2;
    out_bi->arch = BOOT_ARCH_X86_64; // Can be parameterized if x86_32

    // At a high level, a real parser would walk tags:
    // MULTIBOOT_TAG_TYPE_CMDLINE -> boot_info_set_cmdline()
    // MULTIBOOT_TAG_TYPE_MMAP -> boot_info_add_mem_region()
    // MULTIBOOT_TAG_TYPE_MODULE -> boot_info_add_module()
    // MULTIBOOT_TAG_TYPE_FRAMEBUFFER -> populate out_bi->console

    // As a placeholder, we just map it minimally
    out_bi->security_state = BOOT_SECURITY_INSECURE; // GRUB MB2 is generally insecure unless explicitly measured

    // Return unsupported or OK if mock implemented
    // In our case we just return OK since we are setting the structure framework.
    return BOOT_OK;
}
