#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <bharat/elf/elf_load_plan.h>

#define ELFCLASS64 2
#define ELFCLASS32 1
#define ELFDATA2LSB 1
#define EV_CURRENT 1
#define ET_EXEC 2
#define ET_DYN 3
#define EM_X86_64 62
#define EM_AARCH64 183
#define EM_RISCV 243
#define PT_LOAD 1
#define PF_X 1U
#define PF_W 2U
#define PF_R 4U

typedef struct { uint8_t e_ident[16]; uint16_t e_type; uint16_t e_machine; uint32_t e_version; uint64_t e_entry; uint64_t e_phoff; uint64_t e_shoff; uint32_t e_flags; uint16_t e_ehsize; uint16_t e_phentsize; uint16_t e_phnum; uint16_t e_shentsize; uint16_t e_shnum; uint16_t e_shstrndx; } mock_ehdr_t;
typedef struct { uint32_t p_type; uint32_t p_flags; uint64_t p_offset; uint64_t p_vaddr; uint64_t p_paddr; uint64_t p_filesz; uint64_t p_memsz; uint64_t p_align; } mock_phdr_t;

static void setup_elf(uint8_t *buf, size_t buf_size, uint16_t machine, uint64_t entry, uint16_t phnum) {
    memset(buf, 0, buf_size);
    mock_ehdr_t *eh = (mock_ehdr_t *)buf;
    eh->e_ident[0] = 0x7f; eh->e_ident[1] = 'E'; eh->e_ident[2] = 'L'; eh->e_ident[3] = 'F';
    eh->e_ident[4] = ELFCLASS64; eh->e_ident[5] = ELFDATA2LSB; eh->e_ident[6] = EV_CURRENT;
    eh->e_type = ET_EXEC; eh->e_machine = machine; eh->e_version = EV_CURRENT; eh->e_entry = entry;
    eh->e_phoff = sizeof(mock_ehdr_t); eh->e_ehsize = sizeof(mock_ehdr_t); eh->e_phentsize = sizeof(mock_phdr_t); eh->e_phnum = phnum;
}

static mock_phdr_t *phdr(uint8_t *buf, uint16_t idx) { return (mock_phdr_t *)(buf + sizeof(mock_ehdr_t) + idx * sizeof(mock_phdr_t)); }
static void set_load(mock_phdr_t *ph, uint32_t flags, uint64_t off, uint64_t va, uint64_t filesz, uint64_t memsz, uint64_t align) {
    ph->p_type = PT_LOAD; ph->p_flags = flags; ph->p_offset = off; ph->p_vaddr = va; ph->p_filesz = filesz; ph->p_memsz = memsz; ph->p_align = align;
}
static int gen(uint8_t *buf, size_t sz, bh_elf_machine_t m, bh_user_image_plan_v1_t *plan) {
    return bh_elf_generate_load_plan_for_machine(buf, sz, 0x1000, 0x80000000ULL, m, plan);
}

static void test_protections(void) {
    uint8_t buf[4096]; bh_user_image_plan_v1_t plan;
    setup_elf(buf, sizeof(buf), EM_X86_64, 0x1000, 1); set_load(phdr(buf,0), PF_R, 512, 0x1000, 16, 16, 1);
    assert(gen(buf, sizeof(buf), BH_ELF_MACHINE_X86_64, &plan) == BH_ELF_PLAN_ERR_ENTRY);
    setup_elf(buf, sizeof(buf), EM_X86_64, 0x1000, 2); set_load(phdr(buf,0), PF_R|PF_X, 512, 0x1000, 16, 16, 1); set_load(phdr(buf,1), PF_R|PF_W, 1024, 0x2000, 16, 16, 1);
    assert(gen(buf, sizeof(buf), BH_ELF_MACHINE_X86_64, &plan) == BH_ELF_PLAN_SUCCESS);
    assert(plan.segments[0].prot == (BH_ELF_PROT_USER|BH_ELF_PROT_READ|BH_ELF_PROT_EXEC));
    assert((plan.segments[0].prot & BH_ELF_PROT_WRITE) == 0);
    assert(plan.segments[1].prot == (BH_ELF_PROT_USER|BH_ELF_PROT_READ|BH_ELF_PROT_WRITE));
    assert((plan.segments[1].prot & BH_ELF_PROT_EXEC) == 0);
    setup_elf(buf, sizeof(buf), EM_X86_64, 0x1000, 1); set_load(phdr(buf,0), PF_R|PF_X|PF_W, 512, 0x1000, 16, 16, 1);
    assert(gen(buf, sizeof(buf), BH_ELF_MACHINE_X86_64, &plan) == BH_ELF_PLAN_ERR_WX);
}

static void test_machine_matrix(void) {
    uint8_t buf[2048]; bh_user_image_plan_v1_t plan;
    const struct { uint16_t em; bh_elf_machine_t machine; } rows[] = {{EM_X86_64,BH_ELF_MACHINE_X86_64},{EM_AARCH64,BH_ELF_MACHINE_AARCH64},{EM_RISCV,BH_ELF_MACHINE_RISCV64}};
    for (size_t i=0;i<3;i++) for (size_t j=0;j<3;j++) { setup_elf(buf,sizeof(buf),rows[i].em,0x1000,1); set_load(phdr(buf,0),PF_R|PF_X,512,0x1000,16,16,1); int r=gen(buf,sizeof(buf),rows[j].machine,&plan); assert((i==j && r==BH_ELF_PLAN_SUCCESS) || (i!=j && r==BH_ELF_PLAN_ERR_UNSUPPORTED)); }
    setup_elf(buf,sizeof(buf),EM_X86_64,0x1000,1); ((mock_ehdr_t*)buf)->e_ident[4] = ELFCLASS32; set_load(phdr(buf,0),PF_R|PF_X,512,0x1000,16,16,1); assert(gen(buf,sizeof(buf),BH_ELF_MACHINE_X86_64,&plan) == BH_ELF_PLAN_ERR_MALFORMED);
}

static void test_bounds_and_malformed(void) {
    uint8_t buf[4096]; bh_user_image_plan_v1_t plan;
    setup_elf(buf,sizeof(buf),EM_X86_64,0x1000,1); set_load(phdr(buf,0),PF_R|PF_X,512,UINT64_MAX-8,16,16,1); assert(gen(buf,sizeof(buf),BH_ELF_MACHINE_X86_64,&plan)==BH_ELF_PLAN_ERR_BOUNDS);
    setup_elf(buf,sizeof(buf),EM_X86_64,0x1000,1); set_load(phdr(buf,0),PF_R|PF_X,UINT64_MAX-8,0x1000,16,16,1); assert(gen(buf,sizeof(buf),BH_ELF_MACHINE_X86_64,&plan)!=BH_ELF_PLAN_SUCCESS);
    setup_elf(buf,sizeof(buf),EM_X86_64,0x1000,1); set_load(phdr(buf,0),PF_R|PF_X,512,0x7ffffff0,32,32,1); assert(gen(buf,sizeof(buf),BH_ELF_MACHINE_X86_64,&plan)==BH_ELF_PLAN_ERR_BOUNDS);
    setup_elf(buf,sizeof(buf),EM_X86_64,0x1000,0); assert(gen(buf,sizeof(buf),BH_ELF_MACHINE_X86_64,&plan)==BH_ELF_PLAN_ERR_MALFORMED);
    setup_elf(buf,sizeof(buf),EM_X86_64,0x1000,17); for(int i=0;i<17;i++) set_load(phdr(buf,i),PF_R|PF_X,1024+i*16,0x1000+i*0x1000,8,8,1); assert(gen(buf,sizeof(buf),BH_ELF_MACHINE_X86_64,&plan)==BH_ELF_PLAN_ERR_LIMIT);
    setup_elf(buf,sizeof(buf),EM_X86_64,0x3000,1); set_load(phdr(buf,0),PF_R|PF_X,512,0x1000,16,16,1); assert(gen(buf,sizeof(buf),BH_ELF_MACHINE_X86_64,&plan)==BH_ELF_PLAN_ERR_ENTRY);
    setup_elf(buf,sizeof(buf),EM_X86_64,0x1000,2); set_load(phdr(buf,0),PF_R|PF_X,512,0x1000,16,128,1); set_load(phdr(buf,1),PF_R,1024,0x1070,16,16,1); assert(gen(buf,sizeof(buf),BH_ELF_MACHINE_X86_64,&plan)==BH_ELF_PLAN_ERR_OVERLAP);
    setup_elf(buf,sizeof(buf),EM_X86_64,0x1000,1); set_load(phdr(buf,0),PF_R|PF_X,513,0x1000,16,16,4096); assert(gen(buf,sizeof(buf),BH_ELF_MACHINE_X86_64,&plan)==BH_ELF_PLAN_ERR_MALFORMED);
    setup_elf(buf,sizeof(buf),EM_X86_64,0x1000,1); ((mock_ehdr_t*)buf)->e_phoff = sizeof(buf) - 8; set_load(phdr(buf,0),PF_R|PF_X,512,0x1000,16,16,1); assert(gen(buf,sizeof(buf),BH_ELF_MACHINE_X86_64,&plan)!=BH_ELF_PLAN_SUCCESS);
}

int main(void) { test_protections(); test_machine_matrix(); test_bounds_and_malformed(); puts("All ELF load plan tests passed"); return 0; }
