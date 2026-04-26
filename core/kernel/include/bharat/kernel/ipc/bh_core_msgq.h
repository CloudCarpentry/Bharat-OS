#ifndef BHARAT_KERNEL_IPC_BH_CORE_MSGQ_H
#define BHARAT_KERNEL_IPC_BH_CORE_MSGQ_H

#include <stdint.h>
#include <kernel/status.h>
#include <bharat/kernel/ds/bh_ring.h>

/**
 * @file bh_core_msgq.h
 * @brief Bounded Core-to-Core Message Queue
 */

typedef enum {
    BH_CORE_MSG_TLB_INVALIDATE,
    BH_CORE_MSG_SCHED_REMOTE_ENQUEUE,
    BH_CORE_MSG_FAULT_EVENT,
    BH_CORE_MSG_TELEMETRY_EVENT
} bh_core_msg_type_t;

typedef struct {
    uint64_t request_id;
    uint32_t source_core;
    uint32_t target_core;
    bh_core_msg_type_t type;
    uint32_t flags;
    uint64_t arg0;
    uint64_t arg1;
    uint64_t arg2;
} bh_core_msg_t;

typedef struct bh_core_msgq {
    bh_ring_t ring;
} bh_core_msgq_t;

/**
 * @brief Initialize a core message queue.
 *
 * @param q Pointer to the queue.
 * @param storage Pointer to storage for bh_core_msg_t array.
 * @param capacity Number of messages.
 */
void bh_core_msgq_init(bh_core_msgq_t *q, void *storage, uint32_t capacity);

/**
 * @brief Push a message into the queue.
 */
kstatus_t bh_core_msgq_push(bh_core_msgq_t *q, const bh_core_msg_t *msg);

/**
 * @brief Pop a message from the queue.
 */
kstatus_t bh_core_msgq_pop(bh_core_msgq_t *q, bh_core_msg_t *out_msg);

#endif // BHARAT_KERNEL_IPC_BH_CORE_MSGQ_H
