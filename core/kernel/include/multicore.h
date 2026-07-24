#ifndef BHARAT_MULTICORE_H
#define BHARAT_MULTICORE_H

#include <stdint.h>

int multicore_boot_secondary_cores(uint32_t core_count);

#endif // BHARAT_MULTICORE_H

void smp_init(void);
