#include "hal/fdt_parser.h"
#include "hal/hal.h"
#include "hal/hal_boot.h"
#include "boot/boot_info.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define FDT_BEGIN_NODE 0x00000001
#define FDT_END_NODE 0x00000002
#define FDT_PROP 0x00000003
#define FDT_NOP 0x00000004
#define FDT_END 0x00000009

static inline uint32_t fdt32_to_cpu(uint32_t val) {
  return ((val >> 24) & 0xff) | ((val >> 8) & 0xff00) | ((val & 0xff00) << 8) |
         ((val & 0xff) << 24);
}

static int str_eq(const char *a, const char *b) {
  if (!a || !b)
    return 0;
  while (*a != '\0' && *b != '\0') {
    if (*a != *b)
      return 0;
    a++;
    b++;
  }
  return (*a == '\0' && *b == '\0') ? 1 : 0;
}

static int str_starts_with(const char *str, const char *prefix) {
  if (!str || !prefix)
    return 0;
  while (*prefix) {
    if (*str != *prefix)
      return 0;
    str++;
    prefix++;
  }
  return 1;
}

static void* hal_memset(void* s, int c, size_t n) {
    unsigned char* p = (unsigned char*)s;
    while (n--) *p++ = (unsigned char)c;
    return s;
}

static size_t hal_strlen(const char* s) {
    size_t len = 0;
    while (*s++) len++;
    return len;
}

static const char *fdt_get_string(const struct fdt_header *fdt,
                                  uint32_t offset) {
  return (const char *)((uintptr_t)fdt + fdt32_to_cpu(fdt->off_dt_strings) +
                        offset);
}

#define MAX_FDT_DEPTH 32

/*bool fdt_is_valid(const void* fdt_ptr) {
    if (!fdt_ptr) return false;
    const struct fdt_header* fdt = (const struct fdt_header*)fdt_ptr;
    return (fdt32_to_cpu(fdt->magic) == FDT_MAGIC);
}
*/

bool fdt_is_valid(const void *fdt_ptr) {
  if (!fdt_ptr)
    return false;
  const struct fdt_header *fdt = (const struct fdt_header *)fdt_ptr;
  uint32_t magic = fdt32_to_cpu(fdt->magic);
  hal_serial_write("FDT: Checking magic at ");
  hal_serial_write_hex((uintptr_t)fdt_ptr);
  hal_serial_write(" magic=");
  hal_serial_write_hex(magic);
  hal_serial_write("\n");
  return (magic == FDT_MAGIC);
}

// Old legacy function
int fdt_parse(const void *fdt_ptr, void *boot_info_ptr,
              fdt_devices_t *out_devices) {
  if (!fdt_is_valid(fdt_ptr) || !boot_info_ptr || !out_devices) {
    return -1;
  }

  boot_info_t *boot_info = (boot_info_t *)boot_info_ptr;
  const struct fdt_header *fdt = (const struct fdt_header *)fdt_ptr;

  // Clear outputs initially
  boot_info->mem_region_count = 0;

  out_devices->uart_base = 0;
  out_devices->gic_dist_base = 0;
  out_devices->gic_redist_base = 0;
  out_devices->plic_base = 0;
  out_devices->clint_base = 0;

  uint32_t off_dt_struct = fdt32_to_cpu(fdt->off_dt_struct);
  const uint32_t *p = (const uint32_t *)((uintptr_t)fdt + off_dt_struct);

  int depth = 0;
  const char *current_node_name = NULL;

  int address_cells[MAX_FDT_DEPTH] = {2};
  int size_cells[MAX_FDT_DEPTH] = {2};

  int is_memory = 0;
  int is_cpu = 0;
  int is_uart = 0;
  int is_plic = 0;
  int is_clint = 0;
  int is_gic = 0;

  const void *reg_data = NULL;
  uint32_t reg_len = 0;

  while (1) {
    uint32_t tag = fdt32_to_cpu(*p++);
    if (tag == FDT_BEGIN_NODE) {
      current_node_name = (const char *)p;

      // Align to 4 bytes
      size_t len = 0;
      while (current_node_name[len] != '\0')
        len++;
      p += (len + 4) / 4;

      if (depth < MAX_FDT_DEPTH - 1) {
        address_cells[depth + 1] = address_cells[depth];
        size_cells[depth + 1] = size_cells[depth];
        depth++;
      }

      is_memory = str_starts_with(current_node_name, "memory@");
      is_cpu = str_starts_with(current_node_name, "cpu@");
      is_uart = 0;
      is_plic = 0;
      is_clint = 0;
      is_gic = 0;
      reg_data = NULL;
      reg_len = 0;

    } else if (tag == FDT_END_NODE) {
      if (reg_data != NULL) {
        int ac = (depth > 1) ? address_cells[depth - 1] : 2;
        int sc = (depth > 1) ? size_cells[depth - 1] : 2;

        if (reg_len >= (uint32_t)((ac + sc) * 4)) {
          uint64_t base = 0;
          uint64_t size = 0;

          const uint32_t *cell = (const uint32_t *)reg_data;

          if (ac == 2) {
            base =
                ((uint64_t)fdt32_to_cpu(cell[0]) << 32) | fdt32_to_cpu(cell[1]);
            cell += 2;
          } else if (ac == 1) {
            base = fdt32_to_cpu(cell[0]);
            cell += 1;
          }

          if (sc == 2) {
            size =
                ((uint64_t)fdt32_to_cpu(cell[0]) << 32) | fdt32_to_cpu(cell[1]);
            cell += 2;
          } else if (sc == 1) {
            size = fdt32_to_cpu(cell[0]);
            cell += 1;
          }

          if (is_memory &&
              boot_info->mem_region_count < BHARAT_BOOT_MAX_MEM_REGIONS) {
            boot_info->mem_regions[boot_info->mem_region_count].phys_start = base;
            boot_info->mem_regions[boot_info->mem_region_count].size = size;
            boot_info->mem_regions[boot_info->mem_region_count].type =
                BOOT_MEM_USABLE;
            boot_info->mem_region_count++;
          } else if (is_cpu) {
            // Hart IDs / topology handled in riscv_fdt_parse_common
            boot_info->boot_cpu_id = base;
          } else if (is_uart && out_devices->uart_base == 0) {
            out_devices->uart_base = base;
            out_devices->uart_size = size;
          } else if (is_plic && out_devices->plic_base == 0) {
            out_devices->plic_base = base;
            out_devices->plic_size = size;
          } else if (is_clint && out_devices->clint_base == 0) {
            out_devices->clint_base = base;
            out_devices->clint_size = size;
          } else if (is_gic && out_devices->gic_dist_base == 0) {
            out_devices->gic_dist_base = base;
            out_devices->gic_dist_size = size;

            // Parse redistributor if present (second reg tuple)
            if (reg_len >= (uint32_t)((ac + sc) * 8)) {
              if (ac == 2) {
                out_devices->gic_redist_base =
                    ((uint64_t)fdt32_to_cpu(cell[0]) << 32) |
                    fdt32_to_cpu(cell[1]);
                cell += 2;
              } else if (ac == 1) {
                out_devices->gic_redist_base = fdt32_to_cpu(cell[0]);
                cell += 1;
              }
            }
          }
        }
      }

      depth--;
      if (depth <= 0)
        break;
    } else if (tag == FDT_PROP) {
      uint32_t len = fdt32_to_cpu(*p++);
      uint32_t nameoff = fdt32_to_cpu(*p++);
      const char *prop_name = fdt_get_string(fdt, nameoff);
      const void *prop_data = p;

      p += (len + 3) / 4; // Align to 4 bytes

      if (str_eq(prop_name, "device_type") &&
          str_eq((const char *)prop_data, "memory")) {
        is_memory = 1;
      } else if (str_eq(prop_name, "device_type") &&
                 str_eq((const char *)prop_data, "cpu")) {
        is_cpu = 1;
      } else if (str_eq(prop_name, "compatible")) {
        const char *comp = (const char *)prop_data;
        size_t c_len = 0;
        while (c_len < len) {
          if (str_eq(comp + c_len, "ns16550a") ||
              str_eq(comp + c_len, "arm,pl011") ||
              str_eq(comp + c_len, "brcm,bcm2835-aux-uart")) {
            is_uart = 1;
          } else if (str_eq(comp + c_len, "riscv,plic0") ||
                     str_eq(comp + c_len, "sifive,plic-1.0.0")) {
            is_plic = 1;
          } else if (str_eq(comp + c_len, "riscv,clint0") ||
                     str_eq(comp + c_len, "sifive,clint0")) {
            is_clint = 1;
          } else if (str_eq(comp + c_len, "arm,gic-v3") ||
                     str_eq(comp + c_len, "arm,cortex-a15-gic")) {
            is_gic = 1;
          }
          while (c_len < len && comp[c_len] != '\0')
            c_len++;
          c_len++;
        }
      } else if (str_eq(prop_name, "#address-cells")) {
        if (depth < MAX_FDT_DEPTH) {
          address_cells[depth] = fdt32_to_cpu(*(const uint32_t *)prop_data);
        }
      } else if (str_eq(prop_name, "#size-cells")) {
        if (depth < MAX_FDT_DEPTH) {
          size_cells[depth] = fdt32_to_cpu(*(const uint32_t *)prop_data);
        }
      } else if (str_eq(prop_name, "reg")) {
        reg_data = prop_data;
        reg_len = len;
      } else if (str_eq(prop_name, "clock-frequency") && is_cpu) {
        // Ignore for now
      }
    } else if (tag == FDT_NOP) {
      continue;
    } else if (tag == FDT_END) {
      break;
    }
  }

  return 0;
}

typedef struct {
    int is_memory;
    int is_cpu;
    int is_plic;
    int is_gic;
    int is_fb;
    const void *reg_data;
    uint32_t reg_len;
    uint32_t ac;
    uint32_t sc;
} fdt_node_state_t;

// New unified function to parse FDT and fill out discovery structs
int fdt_parse_discovery(const void *fdt_ptr, system_discovery_t *discovery) {
  if (!fdt_is_valid(fdt_ptr) || !discovery) {
    return -1;
  }

  const struct fdt_header *fdt = (const struct fdt_header *)fdt_ptr;
  uint32_t off_dt_struct = fdt32_to_cpu(fdt->off_dt_struct);
  const uint32_t *p = (const uint32_t *)((uintptr_t)fdt + off_dt_struct);
  const uint32_t *end = (const uint32_t *)((uintptr_t)p + fdt32_to_cpu(fdt->size_dt_struct));

  int depth = 0;
  fdt_node_state_t stack[MAX_FDT_DEPTH];
  hal_memset(stack, 0, sizeof(stack));

  // Initialize root state
  stack[0].ac = 2;
  stack[0].sc = 2;

  while (p < end) {
    uint32_t tag = fdt32_to_cpu(*p++);
    if (tag == FDT_BEGIN_NODE) {
      const char *node_name = (const char *)p;
      size_t name_len = hal_strlen(node_name);
      p += (name_len + 4) / 4;

      depth++;
      if (depth >= MAX_FDT_DEPTH) return -1;

      hal_memset(&stack[depth], 0, sizeof(fdt_node_state_t));
      stack[depth].ac = stack[depth-1].ac;
      stack[depth].sc = stack[depth-1].sc;

      if (str_starts_with(node_name, "memory@") || str_eq(node_name, "memory"))
          stack[depth].is_memory = 1;
      if (str_starts_with(node_name, "cpu@") || str_eq(node_name, "cpu"))
          stack[depth].is_cpu = 1;
      if (str_starts_with(node_name, "framebuffer@") || str_eq(node_name, "framebuffer"))
          stack[depth].is_fb = 1;

    } else if (tag == FDT_END_NODE) {
      fdt_node_state_t *s = &stack[depth];
      if (s->reg_data != NULL) {
        const uint32_t *cell = (const uint32_t *)s->reg_data;
        uint64_t base = 0;
        uint64_t size = 0;
        uint32_t ac = stack[depth-1].ac;
        uint32_t sc = stack[depth-1].sc;

        if (ac == 2) {
          base = ((uint64_t)fdt32_to_cpu(cell[0]) << 32) | fdt32_to_cpu(cell[1]);
          cell += 2;
        } else {
          base = fdt32_to_cpu(cell[0]);
          cell += 1;
        }

        if (sc == 2) {
          size = ((uint64_t)fdt32_to_cpu(cell[0]) << 32) | fdt32_to_cpu(cell[1]);
          cell += 2;
        } else if (sc == 1) {
          size = fdt32_to_cpu(cell[0]);
          cell += 1;
        }

        if (s->is_memory && discovery->topology.mem_region_count < BHARAT_MAX_MEM_REGIONS) {
          discovery->topology.mem_regions[discovery->topology.mem_region_count].base = base;
          discovery->topology.mem_regions[discovery->topology.mem_region_count].size = size;
          discovery->topology.mem_regions[discovery->topology.mem_region_count].type = 1;
          discovery->topology.mem_region_count++;
        } else if (s->is_cpu && discovery->topology.cpu_count < BHARAT_MAX_CPUS) {
          discovery->topology.cpus[discovery->topology.cpu_count].hw_id = base;
          discovery->topology.cpu_count++;
        } else if (s->is_gic && discovery->irq_ctrl_count < BHARAT_MAX_IRQ_CONTROLLERS) {
          discovery->irq_ctrls[discovery->irq_ctrl_count].type = IRQ_CTRL_GICV3;
          discovery->irq_ctrls[discovery->irq_ctrl_count].base = base;
          discovery->irq_ctrls[discovery->irq_ctrl_count].size = size;
          // Redistributor check
          if (s->reg_len >= (ac + sc) * 8) {
              if (ac == 2) {
                  discovery->irq_ctrls[discovery->irq_ctrl_count].aux_base = 
                      ((uint64_t)fdt32_to_cpu(cell[0]) << 32) | fdt32_to_cpu(cell[1]);
              } else {
                  discovery->irq_ctrls[discovery->irq_ctrl_count].aux_base = fdt32_to_cpu(cell[0]);
              }
          }
          discovery->irq_ctrl_count++;
        } else if (s->is_plic && discovery->irq_ctrl_count < BHARAT_MAX_IRQ_CONTROLLERS) {
          discovery->irq_ctrls[discovery->irq_ctrl_count].type = IRQ_CTRL_PLIC;
          discovery->irq_ctrls[discovery->irq_ctrl_count].base = base;
          discovery->irq_ctrls[discovery->irq_ctrl_count].size = size;
          discovery->irq_ctrl_count++;
        } else if (s->is_fb) {
          discovery->boot_video.phys_addr = base;
          discovery->boot_video.size = size;
          discovery->boot_video.valid = true;
        }
      }
      depth--;
      if (depth < 0) break;
    } else if (tag == FDT_PROP) {
      uint32_t len = fdt32_to_cpu(*p++);
      uint32_t nameoff = fdt32_to_cpu(*p++);
      const char *prop_name = fdt_get_string(fdt, nameoff);
      const void *prop_data = p;
      p += (len + 3) / 4;

      if (str_eq(prop_name, "reg")) {
        stack[depth].reg_data = prop_data;
        stack[depth].reg_len = len;
      } else if (str_eq(prop_name, "#address-cells")) {
        stack[depth].ac = fdt32_to_cpu(*(const uint32_t *)prop_data);
      } else if (str_eq(prop_name, "#size-cells")) {
        stack[depth].sc = fdt32_to_cpu(*(const uint32_t *)prop_data);
      } else if (str_eq(prop_name, "compatible")) {
        const char *comp = (const char *)prop_data;
        size_t c_len = 0;
        while (c_len < len) {
          if (str_eq(comp + c_len, "arm,gic-v3") || str_eq(comp + c_len, "arm,cortex-a15-gic") ||
              str_eq(comp + c_len, "arm,gic-400") || str_eq(comp + c_len, "arm,gic-v2")) {
            stack[depth].is_gic = 1;
          } else if (str_eq(comp + c_len, "riscv,plic0")) {
            stack[depth].is_plic = 1;
          } else if (str_eq(comp + c_len, "simple-framebuffer")) {
            stack[depth].is_fb = 1;
          }
          while (c_len < len && comp[c_len] != '\0') c_len++;
          c_len++;
        }
      }
    } else if (tag == FDT_END) {
      break;
    }
  }
  return 0;
}
