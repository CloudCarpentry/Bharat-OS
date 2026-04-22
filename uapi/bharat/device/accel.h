#ifndef BHARAT_UAPI_DEVICE_ACCEL_H
#define BHARAT_UAPI_DEVICE_ACCEL_H

#include <stdint.h>
#include <stddef.h>

/**
 * Accelerator Job Descriptor (Tier P)
 * Neutral descriptor for offloading compute tasks to hardware accelerators.
 */
typedef struct accel_job {
    uint64_t job_id;
    uint32_t type;
    uint32_t flags;

    uint64_t input_buffer_handle;
    uint64_t output_buffer_handle;

    uint64_t deadline_ns;
    uint32_t priority;
    uint32_t fault_domain_tag;

    uint8_t  opaque_cmd[64];
} accel_job_t;

/**
 * Accelerator Sync Fence (Tier P)
 */
typedef struct accel_fence {
    uint64_t fence_id;
    volatile uint32_t status;
    uint32_t error_code;
} accel_fence_t;

#define ACCEL_JOB_FLAG_SIGNAL_FENCE (1U << 0)
#define ACCEL_JOB_FLAG_LOW_LATENCY  (1U << 1)

#endif // BHARAT_UAPI_DEVICE_ACCEL_H
