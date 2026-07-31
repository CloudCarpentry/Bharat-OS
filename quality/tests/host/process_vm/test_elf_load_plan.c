#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <bharat/elf/elf_load_plan.h>

#define ELFCLASS64      2
#define ELFDATA2LSB     1
#define EV_CURRENT      1
#define ET_EXEC         2
#define ET_DYN          3
#define EM_X86_64       62

typedef struct {
    uint8_t   e_ident[16];
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
} mock_ehdr_t;

typedef struct {
    uint32_t  p_type;
    uint32_t  p_flags;
    uint64_t  p_offset;
    uint64_t  p_vaddr;
    uint64_t  p_paddr;
    uint64_t  p_filesz;
    uint64_t  p_memsz;
    uint64_t  p_align;
} mock_phdr_t;

void setup_default_elf(uint8_t *buf, size_t buf_size, uint64_t entry, uint64_t vaddr, uint64_t filesz, uint64_t memsz, uint32_t flags) {
    memset(buf, 0, buf_size);
    mock_ehdr_t *ehdr = (mock_ehdr_t *)buf;
    ehdr->e_ident[0] = 0x7f;
    ehdr->e_ident[1] = 'E';
    ehdr->e_ident[2] = 'L';
    ehdr->e_ident[3] = 'F';
    ehdr->e_ident[4] = ELFCLASS64;
    ehdr->e_ident[5] = ELFDATA2LSB;
    ehdr->e_ident[6] = EV_CURRENT;
    ehdr->e_type = ET_EXEC;
    ehdr->e_machine = EM_X86_64;
    ehdr->e_version = EV_CURRENT;
    ehdr->e_entry = entry;
    ehdr->e_phoff = sizeof(mock_ehdr_t);
    ehdr->e_ehsize = sizeof(mock_ehdr_t);
    ehdr->e_phentsize = sizeof(mock_phdr_t);
    ehdr->e_phnum = 1;

    mock_phdr_t *phdr = (mock_phdr_t *)(buf + sizeof(mock_ehdr_t));
    phdr->p_type = 1; // PT_LOAD
    phdr->p_flags = flags;
    phdr->p_offset = sizeof(mock_ehdr_t) + sizeof(mock_phdr_t);
    phdr->p_vaddr = vaddr;
    phdr->p_filesz = filesz;
    phdr->p_memsz = memsz;
    phdr->p_align = 1; // bypassed alignment modulo check
}

void test_load_plan_positive(void) {
    uint8_t buf[1024];
    setup_default_elf(buf, sizeof(buf), 0x1000, 0x1000, 128, 128, 5); // PF_X | PF_R (5)

    bh_user_image_plan_v1_t plan;
    int res = bh_elf_generate_load_plan(buf, sizeof(buf), 0x1000, 0x100000, &plan);
    printf("Debug res: %d\n", res);
    fflush(stdout);
    assert(res == BH_ELF_PLAN_SUCCESS);
    assert(plan.entry_point == 0x1000);
    assert(plan.segment_count == 1);
    assert(plan.segments[0].virtual_address == 0x1000);
    assert(plan.segments[0].file_size == 128);
    printf("test_load_plan_positive passed\n");
}

void test_load_plan_wx_rejection(void) {
    uint8_t buf[1024];
    setup_default_elf(buf, sizeof(buf), 0x1000, 0x1000, 128, 128, 7); // PF_X | PF_W | PF_R (7) - W^X violation

    bh_user_image_plan_v1_t plan;
    int res = bh_elf_generate_load_plan(buf, sizeof(buf), 0x1000, 0x100000, &plan);
    assert(res == BH_ELF_PLAN_ERR_WX);
    printf("test_load_plan_wx_rejection passed\n");
}

void test_load_plan_overlap(void) {
    uint8_t buf[1024];
    setup_default_elf(buf, sizeof(buf), 0x1000, 0x1000, 128, 128, 5);

    // Add a second overlapping program header
    mock_ehdr_t *ehdr = (mock_ehdr_t *)buf;
    ehdr->e_phnum = 2;

    mock_phdr_t *phdr2 = (mock_phdr_t *)(buf + sizeof(mock_ehdr_t) + sizeof(mock_phdr_t));
    phdr2->p_type = 1; // PT_LOAD
    phdr2->p_flags = 5;
    phdr2->p_offset = sizeof(mock_ehdr_t) + 2 * sizeof(mock_phdr_t);
    phdr2->p_vaddr = 0x1050; // Overlaps [0x1000, 0x1080)
    phdr2->p_filesz = 128;
    phdr2->p_memsz = 128;
    phdr2->p_align = 1;

    bh_user_image_plan_v1_t plan;
    int res = bh_elf_generate_load_plan(buf, sizeof(buf), 0x1000, 0x100000, &plan);
    assert(res == BH_ELF_PLAN_ERR_OVERLAP);
    printf("test_load_plan_overlap passed\n");
}

void test_load_plan_out_of_bounds(void) {
    uint8_t buf[1024];
    setup_default_elf(buf, sizeof(buf), 0x1000, 0x1000, 128, 128, 5);

    bh_user_image_plan_v1_t plan;
    int res = bh_elf_generate_load_plan(buf, sizeof(buf), 0x500, 0x2000, &plan); // user_base is 0x500, entry is 0x1000 but segment goes up to 0x1000+128
    assert(res == BH_ELF_PLAN_SUCCESS);

    res = bh_elf_generate_load_plan(buf, sizeof(buf), 0x1500, 0x2000, &plan); // segment is 0x1000, outside user range
    assert(res == BH_ELF_PLAN_ERR_BOUNDS);
    printf("test_load_plan_out_of_bounds passed\n");
}

int main(void) {
    test_load_plan_positive();
    test_load_plan_wx_rejection();
    test_load_plan_overlap();
    test_load_plan_out_of_bounds();
    printf("All ELF load plan tests passed!\n");
    return 0;
}
