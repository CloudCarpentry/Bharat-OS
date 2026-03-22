#ifndef BHARAT_USER_TEST_H
#define BHARAT_USER_TEST_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint64_t cycles;
    uint64_t latency_ns;
    uint64_t errors;
} test_metrics_t;

typedef struct {
    const char* name;
    int (*run)(void);
} test_case_t;

// Framework API
void register_test(const char* name, int (*run)(void));
void run_all_tests(void);
void run_test(const char* name);

// Test declarations
int pmm_stress_test_run(void);
int vmm_stress_test_run(void);

// Syscall / Arch wrappers (stubbed for now, eventually make real syscalls)
uint64_t arch_read_cycles(void);
void console_print(const char* str);
void console_print_metric(const char* test_name, const char* metric_name, uint64_t val);

#endif
