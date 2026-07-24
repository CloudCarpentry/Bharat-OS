#ifndef BHARAT_KERNEL_SCHED_TOPOLOGY_H
#define BHARAT_KERNEL_SCHED_TOPOLOGY_H

#include <stdint.h>
#include <stdbool.h>
#include <bharat/uapi/system/execution_mode.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declaration of per-core runqueue state */
struct sched_runqueue;

/**
 * @brief Extend per-core scheduler state with partition info.
 *
 * Called during boot to bind the validated partition descriptor
 * into the local scheduler context.
 *
 * @param cpu_id The ID of the CPU.
 * @param desc The partition descriptor assigned to this CPU.
 * @return 0 on success.
 */
int sched_topology_bind_cpu(uint32_t cpu_id, const bharat_cpu_partition_desc_t *desc);

/**
 * @brief Verify that a CPU's runqueue complies with its assigned partition rules.
 * @param rq The runqueue to check.
 * @return 0 if valid.
 */
int sched_topology_validate_rq(struct sched_runqueue *rq);

#ifdef __cplusplus
}
#endif

#endif // BHARAT_KERNEL_SCHED_TOPOLOGY_H
