#include "arch/arch_elf.h"

bh_elf_machine_t arch_elf_get_expected_machine(bool *supported) {
    if (supported) {
        *supported = true;
    }
    return BH_ELF_MACHINE_RISCV32;
}
