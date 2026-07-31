#ifndef BHARAT_UAPI_INIT_RT_STARTUP_H
#define BHARAT_UAPI_INIT_RT_STARTUP_H

#include <stdint.h>
#include <bharat/uapi/abi_types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint64_t base;
    uint64_t size;
    uint32_t flags;
    uint32_t reserved;
} bh_rt_region_desc_t;

typedef struct bh_rt_startup {
    uint32_t abi_version;
    uint32_t struct_size;

    uint32_t arch_id;
    uint32_t device_profile;

    uint32_t execution_profile;
    uint32_t memory_model;

    uint32_t cpu_id;
    uint32_t reserved;

    uint64_t timer_frequency;

    // Capability table
    bharat_handle_t self_process_cap;
    bharat_handle_t bootstrap_cap;
    bharat_handle_t diagnostic_channel;
    bharat_handle_t reserved_cap;

    // Region descriptors (e.g. Code, RO-Data, Data, Stack)
    bh_rt_region_desc_t regions[8];
    uint32_t region_count;
    uint32_t flags;
} bh_rt_startup_t;

#ifdef __cplusplus
}
#endif

#endif // BHARAT_UAPI_INIT_RT_STARTUP_H
