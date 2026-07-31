#ifndef BHARAT_ELF_LOAD_PLAN_H
#define BHARAT_ELF_LOAD_PLAN_H

#include <stdint.h>
#include <stddef.h>

#define BH_ELF_PLAN_SUCCESS 0
#define BH_ELF_PLAN_ERR_MALFORMED -1
#define BH_ELF_PLAN_ERR_BOUNDS -2
#define BH_ELF_PLAN_ERR_OVERLAP -3
#define BH_ELF_PLAN_ERR_WX -4
#define BH_ELF_PLAN_ERR_ENTRY -5
#define BH_ELF_PLAN_ERR_UNSUPPORTED -6
#define BH_ELF_PLAN_ERR_LIMIT -7

typedef struct {
    uint64_t virtual_address;
    uint64_t physical_address;
    uint64_t file_offset;
    uint64_t file_size;
    uint64_t memory_size;
    uint32_t flags;
    uint32_t alignment;
    uint32_t prot;
} bh_elf_load_segment_v1_t;

typedef struct {
    uint64_t entry_point;
    uint32_t segment_count;
    bh_elf_load_segment_v1_t segments[16];

    uint64_t stack_size;
    uint64_t guard_size;
    uint64_t startup_page_size;
} bh_user_image_plan_v1_t;

int bh_elf_generate_load_plan(const uint8_t *bytes, size_t size, uint64_t user_base, uint64_t user_limit, bh_user_image_plan_v1_t *out_plan);

#endif // BHARAT_ELF_LOAD_PLAN_H
