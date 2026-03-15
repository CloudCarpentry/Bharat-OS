#!/bin/bash
gcc -Ikernel/include -Ikernel/include/hal -D_GNU_SOURCE -g \
    -Dkmalloc=malloc -Dkfree=free -Dspinlock_init="(void)" -Dspinlock_acquire="(void)" -Dspinlock_release="(void)" \
    -Dhal_get_core_id="hal_get_core_id_mock" \
    kernel/src/tests/test_vm_space.c \
    kernel/src/mm/vm_space.c \
    kernel/src/mm/vm_mapping.c \
    -o test_vm_space -Wno-implicit-function-declaration -Wno-int-conversion

./test_vm_space
