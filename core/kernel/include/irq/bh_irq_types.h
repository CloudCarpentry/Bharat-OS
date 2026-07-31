#ifndef BHARAT_IRQ_TYPES_H
#define BHARAT_IRQ_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include "spinlock.h"
#include "sched/sched.h"
#include "bharat/cpu_local.h"

#define BH_IRQ_MAX_SHARED_ACTIONS 4U

typedef enum {
    BH_IRQ_STATE_FREE = 0,
    BH_IRQ_STATE_ALLOCATED,
    BH_IRQ_STATE_ACTIVE,
    BH_IRQ_STATE_MASKED,
    BH_IRQ_STATE_DISABLING,
    BH_IRQ_STATE_QUIESCING,
    BH_IRQ_STATE_DEAD
} bh_irq_state_t;

typedef enum {
    BH_IRQ_ACTION_STATE_FREE = 0,
    BH_IRQ_ACTION_STATE_ACTIVE,
    BH_IRQ_ACTION_STATE_REMOVING
} bh_irq_action_state_t;

typedef uint64_t bh_irq_handle_t;

// Opaque forward declaration for irq_domain
struct irq_domain;

// Trigger Types
#define BH_IRQ_TRIGGER_NONE     0U
#define BH_IRQ_TRIGGER_RISING   1U
#define BH_IRQ_TRIGGER_FALLING  2U
#define BH_IRQ_TRIGGER_HIGH     4U
#define BH_IRQ_TRIGGER_LOW      8U

typedef struct {
    uint64_t mask;
} bh_irq_affinity_t;

// irq return and flags mapping (for compatibility and core use)
typedef enum {
    BH_IRQ_NONE = 0,
    BH_IRQ_HANDLED = 1,
    BH_IRQ_WAKE_DEFERRED = 2
} bh_irq_return_t;

// Equivalent to old flags
#define BH_IRQF_SHARED     (1U << 0)
#define BH_IRQF_ONESHOT    (1U << 1)
#define BH_IRQF_NO_DEFER   (1U << 2)

typedef bh_irq_return_t (*bh_irq_handler_t)(void* ctx);
typedef void (*bh_irq_deferred_cb_t)(void* ctx);

typedef struct {
    bh_irq_handler_t handler;
    void* ctx;
    void* dev_id;
    const char* name;
    uint32_t flags;
    uint64_t dispatch_count;
    uint32_t active_refs;
    uint32_t state; // bh_irq_action_state_t
} bh_irq_action_t;

// Forward declare bh_irq_controller_ops_t
typedef struct bh_irq_controller_ops bh_irq_controller_ops_t;

struct bh_irq_controller_ops {
    void (*mask)(uint32_t irq);
    void (*unmask)(uint32_t irq);
    void (*ack)(uint32_t irq);
    void (*eoi)(uint32_t irq);
    int (*set_affinity)(uint32_t irq, bh_irq_affinity_t affinity);
    int (*compose_msi_message)(uint32_t irq, uint64_t* msi_address, uint32_t* msi_data);
};

typedef struct {
    uint32_t virq;
    uint32_t generation;

    uint32_t state;       // bh_irq_state_t
    uint32_t in_flight;

    uint32_t flags;
    uint32_t trigger_type;

    bh_irq_affinity_t affinity;
    struct irq_domain *domain;
    const bh_irq_controller_ops_t *controller;

    bh_irq_action_t actions[BH_IRQ_MAX_SHARED_ACTIONS];
    uint32_t action_count;

    uint64_t dispatch_count;
    uint64_t handled_count;
    uint64_t spurious_count;
    uint64_t deferred_count;
    uint64_t dropped_deferred_count;

    spinlock_t lock;
    wait_queue_t quiesce_wait;
} bh_irq_desc_t;

#endif // BHARAT_IRQ_TYPES_H
