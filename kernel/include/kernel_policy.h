#ifndef BHARAT_KERNEL_POLICY_H
#define BHARAT_KERNEL_POLICY_H

#include <bharat/uapi/system/policy.h>
#include <bharat/uapi/system/slo.h>

void kernel_set_system_policy(const bharat_system_policy_t *policy);
const bharat_system_policy_t* kernel_get_system_policy(void);

void kernel_set_slo_gates(const bharat_slo_gates_t *gates);
const bharat_slo_gates_t* kernel_get_slo_gates(void);

#endif // BHARAT_KERNEL_POLICY_H
