#ifndef BHARAT_ARCH_CPU_CAPS_STATE_H
#define BHARAT_ARCH_CPU_CAPS_STATE_H

#include "arch/arch_cpu_caps.h"

void cpu_caps_state_set_boot(const arch_cpu_caps_record_t *caps);
void cpu_caps_state_set_ap(unsigned int cpu_id, const arch_cpu_caps_record_t *caps);

#endif // BHARAT_ARCH_CPU_CAPS_STATE_H
