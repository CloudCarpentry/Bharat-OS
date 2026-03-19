#!/bin/bash
clang -Ikernel/include -Ikernel/include/hal -D_GNU_SOURCE -g \
    -Dkmalloc=malloc -Dkfree=free -Dspinlock_init="(void)" -Dspinlock_acquire="(void)" -Dspinlock_release="(void)" \
    -Dhal_get_core_id="hal_get_core_id_mock" -Dhal_cpu_get_id="hal_get_core_id_mock" \
    kernel/src/tests/test_vm_space.c \
    kernel/src/mm/vm/aspace/vm_space.c \
    kernel/src/mm/legacy/vm_mapping.c \
    kernel/src/cpu_local.c \
    -o test_vm_space -Wno-implicit-function-declaration -Wno-int-conversion

./test_vm_space
