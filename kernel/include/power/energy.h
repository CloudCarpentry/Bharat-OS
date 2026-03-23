#ifndef BHARAT_OS_ENERGY_H
#define BHARAT_OS_ENERGY_H

#include <stdint.h>

typedef struct energy_stats {
    uint64_t total_energy_uj;
    uint32_t current_power_mw;
} energy_stats_t;

int energy_read_stats(int domain_id, energy_stats_t *out_stats);

#endif /* BHARAT_OS_ENERGY_H */
