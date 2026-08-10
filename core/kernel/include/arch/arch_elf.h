#ifndef BHARAT_ARCH_ELF_H
#define BHARAT_ARCH_ELF_H

#include <stdbool.h>
#include <bharat/elf/elf_load_plan.h>

/**
 * @brief Resolves the expected ELF machine type for the current CPU target architecture.
 * Implemented per target architecture in core/arch/<family>/<variant>/arch_elf.c.
 */
bh_elf_machine_t arch_elf_get_expected_machine(bool *supported);

#endif // BHARAT_ARCH_ELF_H
