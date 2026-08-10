#include "boot/adapters/fdt_adapter.h"
#include "boot/boot_errno.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define FDT_BEGIN_NODE 0x00000001
#define FDT_END_NODE 0x00000002
#define FDT_PROP 0x00000003
#define FDT_NOP 0x00000004
#define FDT_END 0x00000009

static inline uint32_t fdt_u32_swap(uint32_t val) {
    return ((val >> 24) & 0xff) | ((val >> 8) & 0xff00) | ((val & 0xff00) << 8) | ((val & 0xff) << 24);
}

static int fdt_str_eq_local(const char *a, const char *b) {
    if (!a || !b) return 0;
    while (*a && *b && *a == *b) { a++; b++; }
    return (*a == *b);
}

static int fdt_str_starts_local(const char *str, const char *prefix) {
    if (!str || !prefix) return 0;
    while (*prefix) {
        if (*str != *prefix) return 0;
        str++; prefix++;
    }
    return 1;
}

struct fdt_hdr_local {
    uint32_t magic;
    uint32_t totalsize;
    uint32_t off_dt_struct;
    uint32_t off_dt_strings;
    uint32_t off_mem_rsvmap;
    uint32_t version;
    uint32_t last_comp_version;
    uint32_t boot_cpuid_phys;
    uint32_t size_dt_strings;
    uint32_t size_dt_struct;
};

int fdt_adapter_parse(const void *fdt_blob, boot_info_t *out_bi) {
    if (!fdt_blob || !out_bi) return BOOT_ERR_INVALID_ARGUMENT;

    const struct fdt_hdr_local *fdt = (const struct fdt_hdr_local *)fdt_blob;
    if (fdt_u32_swap(fdt->magic) != 0xd00dfeed) {
        return BOOT_ERR_BAD_MAGIC;
    }

    boot_info_init(out_bi);
    out_bi->firmware.fdt_ptr = (void *)fdt_blob;

    uint32_t off_dt_struct = fdt_u32_swap(fdt->off_dt_struct);
    uint32_t size_dt_struct = fdt_u32_swap(fdt->size_dt_struct);
    uint32_t off_dt_strings = fdt_u32_swap(fdt->off_dt_strings);

    const uint32_t *p = (const uint32_t *)((uintptr_t)fdt_blob + off_dt_struct);
    const uint32_t *end = (const uint32_t *)((uintptr_t)p + size_dt_struct);

    int depth = 0;
    bool is_chosen = false;
    bool is_memory = false;

    int address_cells[32] = {2};
    int size_cells[32] = {2};

    uint64_t initrd_start = 0;
    uint64_t initrd_end = 0;

    while (p < end) {
        uint32_t tag = fdt_u32_swap(*p++);
        if (tag == FDT_BEGIN_NODE) {
            const char *node_name = (const char *)p;
            size_t name_len = 0;
            while (node_name[name_len] != '\0') {
                name_len++;
            }
            size_t consumed = (name_len + 1U + 3U) & ~3U;
            p = (const uint32_t *)((const uint8_t *)p + consumed);

            depth++;
            if (depth >= 32) return BOOT_ERR_TOO_MANY_MEM_REGIONS;

            address_cells[depth] = address_cells[depth - 1];
            size_cells[depth] = size_cells[depth - 1];

            is_chosen = fdt_str_eq_local(node_name, "chosen") || fdt_str_starts_local(node_name, "chosen@");
            is_memory = fdt_str_eq_local(node_name, "memory") || fdt_str_starts_local(node_name, "memory@");

        } else if (tag == FDT_END_NODE) {
            depth--;
            is_chosen = false;
            is_memory = false;
            if (depth <= 0) break;

        } else if (tag == FDT_PROP) {
            uint32_t len = fdt_u32_swap(*p++);
            uint32_t nameoff = fdt_u32_swap(*p++);
            const char *prop_name = (const char *)((uintptr_t)fdt_blob + off_dt_strings + nameoff);
            const void *prop_data = p;

            size_t consumed = (len + 3U) & ~3U;
            p = (const uint32_t *)((const uint8_t *)p + consumed);

            if (fdt_str_eq_local(prop_name, "#address-cells") && depth < 32) {
                address_cells[depth] = fdt_u32_swap(*(const uint32_t *)prop_data);
            } else if (fdt_str_eq_local(prop_name, "#size-cells") && depth < 32) {
                size_cells[depth] = fdt_u32_swap(*(const uint32_t *)prop_data);
            } else if (is_chosen) {
                if (fdt_str_eq_local(prop_name, "bootargs")) {
                    boot_info_set_cmdline(out_bi, (const char *)prop_data, len);
                } else if (fdt_str_eq_local(prop_name, "linux,initrd-start")) {
                    if (len == 4) {
                        initrd_start = fdt_u32_swap(*(const uint32_t *)prop_data);
                    } else if (len == 8) {
                        const uint32_t *cells = (const uint32_t *)prop_data;
                        initrd_start = ((uint64_t)fdt_u32_swap(cells[0]) << 32) | fdt_u32_swap(cells[1]);
                    }
                } else if (fdt_str_eq_local(prop_name, "linux,initrd-end")) {
                    if (len == 4) {
                        initrd_end = fdt_u32_swap(*(const uint32_t *)prop_data);
                    } else if (len == 8) {
                        const uint32_t *cells = (const uint32_t *)prop_data;
                        initrd_end = ((uint64_t)fdt_u32_swap(cells[0]) << 32) | fdt_u32_swap(cells[1]);
                    }
                }
            } else if (is_memory && fdt_str_eq_local(prop_name, "reg")) {
                int ac = address_cells[depth - 1];
                int sc = size_cells[depth - 1];
                const uint32_t *cells = (const uint32_t *)prop_data;

                uint64_t base = 0;
                uint64_t size = 0;

                if (ac == 2) {
                    base = ((uint64_t)fdt_u32_swap(cells[0]) << 32) | fdt_u32_swap(cells[1]);
                    cells += 2;
                } else {
                    base = fdt_u32_swap(cells[0]);
                    cells += 1;
                }

                if (sc == 2) {
                    size = ((uint64_t)fdt_u32_swap(cells[0]) << 32) | fdt_u32_swap(cells[1]);
                } else {
                    size = fdt_u32_swap(cells[0]);
                }

                boot_info_add_mem_region(out_bi, base, size, BOOT_MEM_USABLE);
            }
        } else if (tag == FDT_NOP) {
            continue;
        } else if (tag == FDT_END) {
            break;
        }
    }

    // Process and validate parsed initrd range
    if (initrd_start != 0 && initrd_end != 0) {
        if (initrd_start >= initrd_end) {
            return BOOT_ERR_INVALID_MEM_RANGE;
        }
        uint64_t size = initrd_end - initrd_start;
        // Bounded sanity check (e.g. initrd size shouldn't exceed 100MB)
        if (size > 100 * 1024 * 1024) {
            return BOOT_ERR_INVALID_MEM_RANGE;
        }

        // Add to normalized boot info as a raw initrd module first.
        // It will be finalized and parsed by bh_boot_handoff_normalize.
        boot_info_add_module(out_bi, initrd_start, size, "initrd");
    }

    return BOOT_OK;
}
