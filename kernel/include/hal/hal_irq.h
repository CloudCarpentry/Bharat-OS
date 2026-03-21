#ifndef BHARAT_HAL_IRQ_H
#define BHARAT_HAL_IRQ_H

#include <stdint.h>
#include <stdbool.h>

// --- IRQ Result Codes ---
typedef enum {
    IRQ_NONE = 0,         // Interrupt was not handled by this handler
    IRQ_HANDLED = 1,      // Interrupt was handled
    IRQ_WAKE_DEFERRED = 2 // Interrupt was handled, and deferred work was scheduled
} irq_return_t;

// --- IRQ Flags ---
#define IRQF_SHARED     (1U << 0) // Allow multiple handlers on this IRQ line
#define IRQF_ONESHOT    (1U << 1) // Keep masked until deferred work finishes
#define IRQF_NO_DEFER   (1U << 2) // Do not defer work (hard IRQ only)

// Forward definition for handler function pointer
typedef irq_return_t (*hal_irq_handler_t)(void* ctx);

// --- Affinity Mask ---
// Define a basic mask type for up to 64 CPUs for now
typedef struct {
    uint64_t mask;
} irq_affinity_mask_t;

// Boot Core Init
void hal_irq_init_boot(void);

// Per-core Init
void hal_irq_init_cpu_local(uint32_t cpu_id);

// Enable / Disable specific vector
int hal_irq_enable(uint32_t vector);
int hal_irq_disable(uint32_t vector);

// Lifecycle
uint32_t hal_irq_claim(void);
void hal_irq_eoi(uint32_t irq);

// Register handler
// `dev_id` is a unique pointer to identify the specific device / handler when unregistering shared IRQs
int hal_interrupt_register(uint32_t irq, hal_irq_handler_t handler, void* ctx, uint32_t flags, const char* name, void* dev_id);
int hal_interrupt_unregister(uint32_t irq, void* dev_id);
void hal_interrupt_dispatch(uint32_t irq);

// Diagnostics
uint64_t hal_interrupt_get_dispatch_count(uint32_t irq);
int hal_interrupt_is_registered(uint32_t irq);

// --- Affinity Mask API ---
int hal_irq_set_affinity(uint32_t irq, irq_affinity_mask_t mask);
int hal_irq_get_affinity(uint32_t irq, irq_affinity_mask_t* mask);
uint32_t hal_irq_pick_target_cpu(uint32_t irq);

// --- Deferred Work (Bottom-Half) ---
typedef struct irq_deferred_work irq_deferred_work_t;

typedef void (*hal_irq_deferred_cb_t)(void* ctx);

struct irq_deferred_work {
    hal_irq_deferred_cb_t callback;
    void* ctx;
    uint32_t source_irq;
    irq_deferred_work_t* next;
};

// Enqueue a deferred work item
int hal_irq_defer(irq_deferred_work_t* work);

// Process the per-core deferred work queue (called on interrupt exit)
void hal_irq_process_deferred(void);


void hal_irq_deferred_init_cpu_local(uint32_t cpu_id);

// --- Controller Abstraction ---
typedef struct irq_controller_ops irq_controller_ops_t;

struct irq_controller_ops {
    void (*mask)(uint32_t irq);
    void (*unmask)(uint32_t irq);
    void (*ack)(uint32_t irq);
    void (*eoi)(uint32_t irq);
    int (*set_affinity)(uint32_t irq, irq_affinity_mask_t mask);

    // Optional MSI compose helper for architectures that support it
    int (*compose_msi_message)(uint32_t irq, uint64_t* msi_address, uint32_t* msi_data);
};

// Set the controller ops for a specific IRQ
int hal_irq_set_controller(uint32_t irq, irq_controller_ops_t* ops);

// Common boot-time initialization for generic IRQ descriptor state.
void hal_irq_generic_init_boot(void);

// Canonical trap-side helper that preserves current acknowledge/eoi contracts
// while allowing dispatch unification at a single call site.
typedef void (*hal_irq_dispatch_fn_t)(uint32_t irq, void* ctx);
void hal_interrupt_handle_trap_irq(uint64_t hw_cause,
                                   void (*timer_handler)(void),
                                   hal_irq_dispatch_fn_t dispatch_fn,
                                   void* dispatch_ctx);

// Returns architecture-appropriate timer interrupt identifier used by trap code.
uint64_t hal_irq_timer_vector(void);

#endif // BHARAT_HAL_IRQ_H
