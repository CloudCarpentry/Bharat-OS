#include "hal/riscv_bsp.h"

#include <stddef.h>
#include <stdint.h>

/* Minimal FDT Definitions */
#define FDT_MAGIC 0xd00dfeed
#define FDT_BEGIN_NODE 0x00000001
#define FDT_END_NODE   0x00000002
#define FDT_PROP       0x00000003
#define FDT_NOP        0x00000004
#define FDT_END        0x00000009

struct fdt_header {
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

static inline uint32_t fdt32_to_cpu(uint32_t val) {
    return ((val >> 24) & 0xff) |
           ((val >> 8) & 0xff00) |
           ((val & 0xff00) << 8) |
           ((val & 0xff) << 24);
}

static riscv_bsp_config_t g_active_cfg;

static int str_eq(const char* a, const char* b) {
    if (!a || !b) {
        return 0;
    }

    while (*a != '\0' && *b != '\0') {
        if (*a != *b) {
            return 0;
        }
        a++;
        b++;
    }

    return (*a == '\0' && *b == '\0') ? 1 : 0;
}

static void cfg_clear(riscv_bsp_config_t* cfg) {
    if (!cfg) {
        return;
    }

    cfg->soc_profile = RISCV_SOC_QEMU_VIRT;
    cfg->fdt_ptr = 0ULL;
    cfg->uart_base = 0ULL;
    cfg->plic_base = 0ULL;
    cfg->clint_base = 0ULL;
    cfg->dram_base = 0ULL;
    cfg->dram_size = 0ULL;
}

static int str_starts_with(const char* str, const char* prefix) {
    if (!str || !prefix) return 0;
    while (*prefix) {
        if (*str != *prefix) return 0;
        str++;
        prefix++;
    }
    return 1;
}

static const char* fdt_get_string(const struct fdt_header* fdt, uint32_t offset) {
    return (const char*)((uintptr_t)fdt + fdt32_to_cpu(fdt->off_dt_strings) + offset);
}

#define MAX_FDT_DEPTH 16

/*
 * Lightweight FDT parser to find specific devices based on compatible string or device_type.
 * This is a minimal implementation, and works for standard flat device trees.
 */
static void parse_fdt(uint64_t fdt_ptr, riscv_bsp_config_t* cfg) {
    if (fdt_ptr == 0ULL) return;

    const struct fdt_header* fdt = (const struct fdt_header*)fdt_ptr;
    if (fdt32_to_cpu(fdt->magic) != FDT_MAGIC) return;

    uint32_t off_dt_struct = fdt32_to_cpu(fdt->off_dt_struct);
    const uint32_t* p = (const uint32_t*)((uintptr_t)fdt + off_dt_struct);

    int depth = 0;
    const char* current_node_name = NULL;

    int address_cells[MAX_FDT_DEPTH] = {2}; // Stack to track #address-cells
    int size_cells[MAX_FDT_DEPTH] = {2};    // Stack to track #size-cells

    address_cells[0] = 2; // Default for root
    size_cells[0] = 2; // Default for root

    int is_memory = 0;
    int is_uart = 0;
    int is_plic = 0;
    int is_clint = 0;
    const void* reg_data = NULL;
    uint32_t reg_len = 0;

    while (1) {
        uint32_t tag = fdt32_to_cpu(*p++);
        if (tag == FDT_BEGIN_NODE) {
            current_node_name = (const char*)p;

            // Align to 4 bytes
            size_t len = 0;
            while (current_node_name[len] != '\0') len++;
            p += (len + 4) / 4;

            if (depth < MAX_FDT_DEPTH - 1) {
                address_cells[depth + 1] = address_cells[depth]; // Inherit by default
                size_cells[depth + 1] = size_cells[depth];       // Inherit by default
            }
            depth++;

            is_memory = str_starts_with(current_node_name, "memory@");
            is_uart = 0;
            is_plic = 0;
            is_clint = 0;
            reg_data = NULL;
            reg_len = 0;

        } else if (tag == FDT_END_NODE) {
            // Process node properties when ending the node
            if (reg_data != NULL) {
                int ac = (depth > 1) ? address_cells[depth - 1] : 2; // Cells are defined by parent
                int sc = (depth > 1) ? size_cells[depth - 1] : 2;    // Cells are defined by parent

                // Need enough reg data
                if (reg_len >= (uint32_t)((ac + sc) * 4)) {
                    uint64_t base = 0;
                    uint64_t size = 0;

                    const uint32_t* cell = (const uint32_t*)reg_data;

                    if (ac == 2) {
                        base = ((uint64_t)fdt32_to_cpu(cell[0]) << 32) | fdt32_to_cpu(cell[1]);
                        cell += 2;
                    } else if (ac == 1) {
                        base = fdt32_to_cpu(cell[0]);
                        cell += 1;
                    }

                    if (sc == 2) {
                        size = ((uint64_t)fdt32_to_cpu(cell[0]) << 32) | fdt32_to_cpu(cell[1]);
                    } else if (sc == 1) {
                        size = fdt32_to_cpu(cell[0]);
                    }

                    if (is_memory && cfg->dram_base == 0) {
                        cfg->dram_base = base;
                        cfg->dram_size = size;
                    } else if (is_uart && cfg->uart_base == 0) {
                        cfg->uart_base = base;
                    } else if (is_plic && cfg->plic_base == 0) {
                        cfg->plic_base = base;
                    } else if (is_clint && cfg->clint_base == 0) {
                        cfg->clint_base = base;
                    }
                }
            }

            depth--;
            if (depth == 0) break;
        } else if (tag == FDT_PROP) {
            uint32_t len = fdt32_to_cpu(*p++);
            uint32_t nameoff = fdt32_to_cpu(*p++);
            const char* prop_name = fdt_get_string(fdt, nameoff);
            const void* prop_data = p;

            p += (len + 3) / 4; // Align to 4 bytes

            if (str_eq(prop_name, "device_type") && str_eq((const char*)prop_data, "memory")) {
                is_memory = 1;
            } else if (str_eq(prop_name, "compatible")) {
                const char* comp = (const char*)prop_data;
                // Parse compatible list properly if needed, but a simple substring check suffices for minimal
                size_t c_len = 0;
                while(c_len < len) {
                     if (str_eq(comp + c_len, "ns16550a")) {
                         is_uart = 1;
                     } else if (str_eq(comp + c_len, "riscv,plic0") || str_eq(comp + c_len, "sifive,plic-1.0.0")) {
                         is_plic = 1;
                     } else if (str_eq(comp + c_len, "riscv,clint0") || str_eq(comp + c_len, "sifive,clint0")) {
                         is_clint = 1;
                     }
                     while(comp[c_len] != '\0' && c_len < len) c_len++;
                     c_len++;
                }
            } else if (str_eq(prop_name, "#address-cells")) {
                if (depth < MAX_FDT_DEPTH) {
                    address_cells[depth] = fdt32_to_cpu(*(const uint32_t*)prop_data);
                }
            } else if (str_eq(prop_name, "#size-cells")) {
                if (depth < MAX_FDT_DEPTH) {
                    size_cells[depth] = fdt32_to_cpu(*(const uint32_t*)prop_data);
                }
            } else if (str_eq(prop_name, "reg")) {
                reg_data = prop_data;
                reg_len = len;
            }
        } else if (tag == FDT_NOP) {
            continue;
        } else if (tag == FDT_END) {
            break;
        }
    }
}

int hal_riscv_bsp_detect(const char* profile, uint64_t fdt_ptr, riscv_bsp_config_t* out_cfg) {
    if (!profile || !out_cfg) {
        return -1;
    }

    cfg_clear(out_cfg);
    out_cfg->fdt_ptr = fdt_ptr;

    /*
     * Baseline board descriptors for early boot.
     * Runtime FDT parsing is planned to supersede these constants.
     */
    if (str_eq(profile, "shakti-e")) {
        out_cfg->soc_profile = RISCV_SOC_SHAKTI_E;
        out_cfg->uart_base = 0xF0000000ULL;
        out_cfg->plic_base = 0x0C000000ULL;
        out_cfg->clint_base = 0x02000000ULL;
        out_cfg->dram_base = 0x80000000ULL;
        out_cfg->dram_size = 0x10000000ULL; /* 256 MiB baseline */
        return 0;
    }

    if (str_eq(profile, "shakti-c")) {
        out_cfg->soc_profile = RISCV_SOC_SHAKTI_C;
        out_cfg->uart_base = 0xE0001800ULL;
        out_cfg->plic_base = 0x0C000000ULL;
        out_cfg->clint_base = 0x02000000ULL;
        out_cfg->dram_base = 0x80000000ULL;
        out_cfg->dram_size = 0x40000000ULL; /* 1 GiB baseline */
        return 0;
    }

    if (str_eq(profile, "shakti-i")) {
        out_cfg->soc_profile = RISCV_SOC_SHAKTI_I;
        out_cfg->uart_base = 0x20000000ULL;
        out_cfg->plic_base = 0x0C000000ULL;
        out_cfg->clint_base = 0x02000000ULL;
        out_cfg->dram_base = 0x80000000ULL;
        out_cfg->dram_size = 0x20000000ULL; /* 512 MiB baseline */
    } else {
        /* Default QEMU virt profile used for current runtime CI. */
        out_cfg->soc_profile = RISCV_SOC_QEMU_VIRT;
        out_cfg->uart_base = 0x10000000ULL;
        out_cfg->plic_base = 0x0C000000ULL;
        out_cfg->clint_base = 0x02000000ULL;
        out_cfg->dram_base = 0x80000000ULL;
        out_cfg->dram_size = 0x08000000ULL; /* 128 MiB baseline */
    }

    /* Override with FDT if present */
    if (fdt_ptr != 0ULL) {
        riscv_bsp_config_t fdt_cfg;
        cfg_clear(&fdt_cfg);
        fdt_cfg.soc_profile = out_cfg->soc_profile;
        fdt_cfg.fdt_ptr = fdt_ptr;
        parse_fdt(fdt_ptr, &fdt_cfg);

        if (fdt_cfg.uart_base != 0) out_cfg->uart_base = fdt_cfg.uart_base;
        if (fdt_cfg.plic_base != 0) out_cfg->plic_base = fdt_cfg.plic_base;
        if (fdt_cfg.clint_base != 0) out_cfg->clint_base = fdt_cfg.clint_base;
        if (fdt_cfg.dram_base != 0) out_cfg->dram_base = fdt_cfg.dram_base;
        if (fdt_cfg.dram_size != 0) out_cfg->dram_size = fdt_cfg.dram_size;
    }

    return 0;
}

int hal_riscv_bsp_init(const riscv_bsp_config_t* cfg) {
    if (!cfg) {
        return -1;
    }

    if (cfg->uart_base == 0ULL || cfg->plic_base == 0ULL || cfg->clint_base == 0ULL) {
        return -1;
    }

    g_active_cfg = *cfg;
    return 0;
}

const riscv_bsp_config_t* hal_riscv_bsp_active(void) {
    return &g_active_cfg;
}
