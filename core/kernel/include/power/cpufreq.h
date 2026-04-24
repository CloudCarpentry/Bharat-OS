#ifndef BHARAT_OS_CPUFREQ_H
#define BHARAT_OS_CPUFREQ_H

#include "power/perf_domain.h"

int cpufreq_register_domain(int cpu_id, perf_domain_t *domain);
int cpufreq_set_frequency(int cpu_id, uint32_t freq_khz);
int cpufreq_get_frequency(int cpu_id, uint32_t *freq_khz);

#endif /* BHARAT_OS_CPUFREQ_H */
