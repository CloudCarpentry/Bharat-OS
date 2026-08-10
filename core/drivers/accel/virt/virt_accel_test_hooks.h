#ifndef VIRT_ACCEL_TEST_HOOKS_H
#define VIRT_ACCEL_TEST_HOOKS_H

#include <stdbool.h>
#include <stdint.h>
#include <bharat/accel/accel.h>

// Shared diagnostics and hooks for testing/demo
uint64_t virt_accel_get_submit_count(void);
void virt_accel_reset_submit_count(void);
void virt_accel_set_fail_injection(bool enable);

uint64_t virt_gpu_get_submit_count(void);
void virt_gpu_reset_submit_count(void);
void virt_gpu_set_fail_injection(bool enable);

bharat_accel_device_t* get_virt_gpu_mock_device(void);

#endif // VIRT_ACCEL_TEST_HOOKS_H
