#ifndef BHARAT_UAPI_SHELL_DIAGNOSTIC_H
#define BHARAT_UAPI_SHELL_DIAGNOSTIC_H

#include <stddef.h>
#include <stdint.h>

#define BH_SHELL_DIAG_ABI_VERSION 1u
#define BH_SHELL_DIAG_MAX_ARGUMENT 96u
#define BH_SHELL_TIMER_SOURCE_NAME_MAX 32u

typedef enum bh_shell_diag_query {
  BH_SHELL_DIAG_CPU_INFO = 1,
  BH_SHELL_DIAG_HW_CAP = 2,
  BH_SHELL_DIAG_PROCESS_LIST = 3,
  BH_SHELL_DIAG_SERVICE_LIST = 4,
  BH_SHELL_DIAG_SERVICE_STATUS = 5,
  BH_SHELL_DIAG_MEMORY_INFO = 6,
  BH_SHELL_DIAG_VM_STAT = 7,
  BH_SHELL_DIAG_HMEM_INFO = 8,
  BH_SHELL_DIAG_HMEM_TOPOLOGY = 9,
  BH_SHELL_DIAG_HMEM_STATS = 10,
  BH_SHELL_DIAG_HMEM_ALLOC = 11,
  BH_SHELL_DIAG_HMEM_BENCH = 12,
  BH_SHELL_DIAG_TENSOR_INFO = 13,
  BH_SHELL_DIAG_TENSOR_BENCH = 14,
  BH_SHELL_DIAG_TIMER_INFO = 15,
  BH_SHELL_DIAG_CAP_STAT = 16,
  BH_SHELL_DIAG_DEVICE_LIST = 17,
  BH_SHELL_DIAG_IO_STAT = 18,
  BH_SHELL_DIAG_NET_STATUS = 19,
  BH_SHELL_DIAG_RUN = 20,
  BH_SHELL_DIAG_TIMER_TEST = 21
} bh_shell_diag_query_t;

/* Fixed-width, by-value request. The receiving service owns all discovered
 * state. */
typedef struct bh_shell_diag_request_v1 {
  uint16_t abi_version;
  uint16_t struct_size;
  uint32_t query;
  uint32_t argument_length;
  uint32_t reserved;
  char argument[BH_SHELL_DIAG_MAX_ARGUMENT];
} bh_shell_diag_request_v1_t;

_Static_assert(sizeof(bh_shell_diag_request_v1_t) == 112u,
               "shell diagnostic request ABI size changed");
_Static_assert(offsetof(bh_shell_diag_request_v1_t, argument) == 16u,
               "shell diagnostic request ABI layout changed");

/* Service-owned timer evidence. Values are measured on the running target;
 * UINT64_MAX means that a measurement is unavailable, never a fabricated 0. */
typedef struct bh_shell_timer_report_v1 {
  uint16_t abi_version;
  uint16_t struct_size;
  uint32_t online_cores;
  uint64_t resolution_ns;
  uint64_t cross_core_skew_ns;
  uint64_t deadline_error_ns;
  uint64_t interrupt_jitter_ns;
  uint64_t scheduler_wakeup_latency_ns;
  uint32_t monotonicity_samples;
  uint32_t monotonicity_failures;
  char clock_source[BH_SHELL_TIMER_SOURCE_NAME_MAX];
} bh_shell_timer_report_v1_t;

_Static_assert(sizeof(bh_shell_timer_report_v1_t) == 88u,
               "shell timer report ABI size changed");
_Static_assert(offsetof(bh_shell_timer_report_v1_t, clock_source) == 56u,
               "shell timer report ABI layout changed");

#endif
