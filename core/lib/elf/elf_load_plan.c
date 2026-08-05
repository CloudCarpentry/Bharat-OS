#include <bharat/elf/elf_load_plan.h>
#include <bharat/elf/elf_parser.h>
#include <stdbool.h>

#define EI_NIDENT       16
#define EI_CLASS        4
#define EI_DATA         5
#define EI_VERSION      6

#define ELFCLASS64      2
#define ELFDATA2LSB     1
#define EV_CURRENT      1
#define ET_EXEC         2
#define ET_DYN          3

#define EM_X86_64       62
#define EM_AARCH64      183
#define EM_RISCV        243

static void plan_zero(void *ptr, size_t size) {
    uint8_t *p = (uint8_t *)ptr;
    for (size_t i = 0; i < size; ++i) {
        p[i] = 0;
    }
}

typedef struct {
    uint8_t   e_ident[EI_NIDENT];
    uint16_t  e_type;
    uint16_t  e_machine;
    uint32_t  e_version;
    uint64_t  e_entry;
    uint64_t  e_phoff;
    uint64_t  e_shoff;
    uint32_t  e_flags;
    uint16_t  e_ehsize;
    uint16_t  e_phentsize;
    uint16_t  e_phnum;
    uint16_t  e_shentsize;
    uint16_t  e_shnum;
    uint16_t  e_shstrndx;
} local_elf64_ehdr_t;

static bool machine_matches(uint16_t elf_machine, bh_elf_machine_t expected_machine) {
    switch (expected_machine) {
    case BH_ELF_MACHINE_X86_64:
        return elf_machine == EM_X86_64;
    case BH_ELF_MACHINE_AARCH64:
        return elf_machine == EM_AARCH64;
    case BH_ELF_MACHINE_RISCV64:
        return elf_machine == EM_RISCV;
    default:
        return false;
    }
}

int bh_elf_generate_load_plan_for_machine(const uint8_t *bytes, size_t size, uint64_t user_base, uint64_t user_limit, bh_elf_machine_t expected_machine, bh_user_image_plan_v1_t *out_plan) {
    if (out_plan) {
        plan_zero(out_plan, sizeof(*out_plan));
    }

    if (!bytes || !out_plan) {
        return BH_ELF_PLAN_ERR_MALFORMED;
    }

    if (size < sizeof(local_elf64_ehdr_t)) {
        return BH_ELF_PLAN_ERR_MALFORMED;
    }

    const local_elf64_ehdr_t *ehdr = (const local_elf64_ehdr_t *)bytes;

    // Check basic headers
    if (ehdr->e_ident[0] != 0x7f || ehdr->e_ident[1] != 'E' || ehdr->e_ident[2] != 'L' || ehdr->e_ident[3] != 'F') {
        return BH_ELF_PLAN_ERR_MALFORMED;
    }

    if (ehdr->e_ident[EI_CLASS] != ELFCLASS64 || ehdr->e_ident[EI_DATA] != ELFDATA2LSB) {
        return BH_ELF_PLAN_ERR_MALFORMED;
    }

    if (!machine_matches(ehdr->e_machine, expected_machine)) {
        return BH_ELF_PLAN_ERR_UNSUPPORTED;
    }

    // ET_DYN policy (shared/PIE reject)
    if (ehdr->e_type == ET_DYN) {
        return BH_ELF_PLAN_ERR_UNSUPPORTED;
    }

    if (ehdr->e_type != ET_EXEC) {
        return BH_ELF_PLAN_ERR_MALFORMED;
    }

    // Parse image summary
    elf_summary_t summary;
    elf_parse_status_t p_status = elf_parse_image(bytes, size, &summary);
    if (p_status != ELF_PARSE_OK) {
        return BH_ELF_PLAN_ERR_MALFORMED;
    }

    size_t seg_count = 0;
    p_status = elf_get_load_segment_count(bytes, size, &seg_count);
    if (p_status != ELF_PARSE_OK || seg_count == 0) {
        return BH_ELF_PLAN_ERR_MALFORMED;
    }

    if (seg_count > 16) {
        return BH_ELF_PLAN_ERR_LIMIT;
    }

    elf_segment_t raw_segments[16];
    size_t extracted = 0;
    p_status = elf_extract_load_segments(bytes, size, raw_segments, 16, &extracted);
    if (p_status != ELF_PARSE_OK || extracted != seg_count) {
        return BH_ELF_PLAN_ERR_MALFORMED;
    }

    out_plan->entry_point = summary.entry_point;
    out_plan->segment_count = (uint32_t)seg_count;
    out_plan->stack_size = 64 * 1024;
    out_plan->guard_size = 4 * 1024;
    out_plan->startup_page_size = 4 * 1024;

    bool entry_validated = false;

    for (size_t i = 0; i < seg_count; ++i) {
        elf_segment_t *seg = &raw_segments[i];

        // Validate basic sizing
        if (seg->file_size > seg->memory_size) {
            return BH_ELF_PLAN_ERR_MALFORMED;
        }

        // Check for bounds overflow
        if (seg->file_offset > size || seg->file_size > size - seg->file_offset) {
            return BH_ELF_PLAN_ERR_BOUNDS;
        }

        // Check user-address range bounds and overflow
        uint64_t v_start = seg->virtual_address;
        uint64_t v_end = v_start + seg->memory_size;
        if (v_end < v_start) {
            return BH_ELF_PLAN_ERR_BOUNDS;
        }

        if (v_start < user_base || v_end > user_limit) {
            return BH_ELF_PLAN_ERR_BOUNDS;
        }

        // W^X Enforcement
        bool is_write = (seg->flags & 2) != 0; // PF_W
        bool is_exec = (seg->flags & 1) != 0;  // PF_X
        if (is_write && is_exec) {
            return BH_ELF_PLAN_ERR_WX;
        }

        // Is entry point inside this executable segment?
        if (out_plan->entry_point >= v_start && out_plan->entry_point < v_start + seg->memory_size) {
            if (is_exec) {
                entry_validated = true;
            }
        }

        if (seg->alignment > 1 && (seg->virtual_address % seg->alignment) != (seg->file_offset % seg->alignment)) {
            return BH_ELF_PLAN_ERR_MALFORMED;
        }

        uint32_t prot = BH_ELF_PROT_USER;
        if ((seg->flags & 4U) != 0U) prot |= BH_ELF_PROT_READ;
        if ((seg->flags & 2U) != 0U) prot |= BH_ELF_PROT_WRITE;
        if ((seg->flags & 1U) != 0U) prot |= BH_ELF_PROT_EXEC;

        out_plan->segments[i].virtual_address = seg->virtual_address;
        out_plan->segments[i].physical_address = seg->physical_address;
        out_plan->segments[i].file_offset = seg->file_offset;
        out_plan->segments[i].file_size = seg->file_size;
        out_plan->segments[i].memory_size = seg->memory_size;
        out_plan->segments[i].flags = seg->flags;
        out_plan->segments[i].alignment = (uint32_t)seg->alignment;
        out_plan->segments[i].prot = prot;
    }

    // Overlapping PT_LOAD segments check
    for (size_t i = 0; i < seg_count; ++i) {
        uint64_t i_start = out_plan->segments[i].virtual_address;
        uint64_t i_end = i_start + out_plan->segments[i].memory_size;

        for (size_t j = i + 1; j < seg_count; ++j) {
            uint64_t j_start = out_plan->segments[j].virtual_address;
            uint64_t j_end = j_start + out_plan->segments[j].memory_size;

            if (i_start < j_end && j_start < i_end) {
                return BH_ELF_PLAN_ERR_OVERLAP;
            }
        }
    }

    if (!entry_validated) {
        return BH_ELF_PLAN_ERR_ENTRY;
    }

    return BH_ELF_PLAN_SUCCESS;
}

int bh_elf_generate_load_plan(const uint8_t *bytes, size_t size, uint64_t user_base, uint64_t user_limit, bh_user_image_plan_v1_t *out_plan) {
    return bh_elf_generate_load_plan_for_machine(bytes, size, user_base, user_limit, BH_ELF_MACHINE_X86_64, out_plan);
}
