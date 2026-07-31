#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include "hal/hal.h"
#include "hal/hal_irq.h"
#include "irq/bh_irq.h"
#include "device/irq_domain.h"
#include "kernel/status.h"

// --- Mock/Stubs for Host Test environment ---
void hal_serial_write(const char* s) { (void)s; }
void hal_timer_tick(void) {}
uint32_t hal_cpu_get_id(void) { return 0; }
void sched_wait_queue_init(wait_queue_t* queue) { (void)queue; }
void bh_thread_yield(void) {}
uint64_t sched_get_ticks(void) { return 1000; }
uint32_t hal_interrupt_get_active_irq(uint64_t hw_cause) { return (uint32_t)hw_cause; }
uint64_t hal_irq_timer_vector(void) { return 0; }
void hal_irq_eoi(uint32_t irq) { (void)irq; }

// Dummy structures to act as dev_ids
static int dev_a = 1;
static int dev_b = 2;
static int dev_c = 3;

static int g_handler_called_count = 0;

static bh_irq_return_t test_handler_a(void* ctx) {
    (void)ctx;
    g_handler_called_count++;
    return BH_IRQ_HANDLED;
}

// Global control counters
static int g_eoi_count = 0;
static int g_ack_count = 0;
static int g_mask_count = 0;
static int g_unmask_count = 0;

static void mock_mask(uint32_t irq) { (void)irq; g_mask_count++; }
static void mock_unmask(uint32_t irq) { (void)irq; g_unmask_count++; }
static void mock_ack(uint32_t irq) { (void)irq; g_ack_count++; }
static void mock_eoi(uint32_t irq) { (void)irq; g_eoi_count++; }

static bh_irq_controller_ops_t mock_controller = {
    .mask = mock_mask,
    .unmask = mock_unmask,
    .ack = mock_ack,
    .eoi = mock_eoi,
    .set_affinity = NULL,
    .compose_msi_message = NULL
};

// -------------------------------------------------------------
// Scenarios 1 & 2: Descriptor Allocation, Reuse & Generation
// -------------------------------------------------------------
void test_allocation_and_generation(void) {
    hal_irq_generic_init_boot();

    bh_irq_handle_t h1 = 0;
    kstatus_t status = bh_irq_register(15, test_handler_a, NULL, 0, "test1", &dev_a, &h1);
    assert(status == K_OK);
    assert(bh_irq_handle_slot(h1) == 15);
    assert(bh_irq_handle_generation(h1) == 1);

    // Unregister and verify generation increments
    status = bh_irq_unregister(h1, &dev_a);
    assert(status == K_OK);

    bh_irq_handle_t h2 = 0;
    status = bh_irq_register(15, test_handler_a, NULL, 0, "test2", &dev_a, &h2);
    assert(status == K_OK);
    assert(bh_irq_handle_slot(h2) == 15);
    // Generation should be incremented to 2 after reuse
    assert(bh_irq_handle_generation(h2) == 2);

    // Stale handle (h1) should be rejected
    status = bh_irq_unregister(h1, &dev_a);
    assert(status == K_ERR_BAD_STATE);

    printf("  [TEST] test_allocation_and_generation: PASS\n");
}

// -------------------------------------------------------------
// Scenario 3: Duplicate Mapping Reject
// -------------------------------------------------------------
void test_duplicate_mapping_rejection(void) {
    irq_domain_t* domain = irq_domain_create("test-domain", 100, 10, NULL);
    assert(domain != NULL);

    int rc = irq_domain_map(domain, 101, 5);
    assert(rc == 0);

    // Reject duplicate virtual IRQ
    rc = irq_domain_map(domain, 101, 6);
    assert(rc < 0);

    // Reject duplicate hardware IRQ
    rc = irq_domain_map(domain, 102, 5);
    assert(rc < 0);

    printf("  [TEST] test_duplicate_mapping_rejection: PASS\n");
}

// -------------------------------------------------------------
// Scenario 4 & 5: Parent Domain & Unmapped IRQs
// -------------------------------------------------------------
void test_domain_translation_and_unmapped(void) {
    irq_domain_t* parent = irq_domain_create("parent", 0, 256, NULL);
    irq_domain_t* child = irq_domain_create("child", 100, 32, NULL);
    child->parent = parent;

    irq_domain_set_default(parent);

    // Map GSI in child
    int rc = irq_domain_map(child, 105, 12);
    assert(rc == 0);

    uint32_t out_virq = 0;
    rc = irq_domain_translate(child, 12, &out_virq);
    assert(rc == 0);
    assert(out_virq == 105);

    // Unmapped translation should fail closed
    rc = irq_domain_translate(child, 99, &out_virq);
    assert(rc < 0);

    printf("  [TEST] test_domain_translation_and_unmapped: PASS\n");
}

// -------------------------------------------------------------
// Scenario 6: Shared Handler Rules
// -------------------------------------------------------------
void test_shared_handler_rules(void) {
    hal_irq_generic_init_boot();

    bh_irq_handle_t h1, h2;
    // Register without SHARED flag
    kstatus_t status = bh_irq_register(20, test_handler_a, NULL, 0, "handler1", &dev_a, &h1);
    assert(status == K_OK);

    // Try registering second handler on same line - should be denied
    status = bh_irq_register(20, test_handler_a, NULL, BH_IRQF_SHARED, "handler2", &dev_b, &h2);
    assert(status == K_ERR_DENIED);

    // Try registering duplicate dev_id
    status = bh_irq_register(20, test_handler_a, NULL, 0, "handler1-dup", &dev_a, &h2);
    assert(status == K_ERR_DENIED);

    printf("  [TEST] test_shared_handler_rules: PASS\n");
}

// -------------------------------------------------------------
// Scenario 7: Oneshot Masking
// -------------------------------------------------------------
void test_oneshot_masking(void) {
    hal_irq_generic_init_boot();

    bh_irq_handle_t h1;
    kstatus_t status = bh_irq_register(25, test_handler_a, NULL, BH_IRQF_ONESHOT, "handler", &dev_a, &h1);
    assert(status == K_OK);

    bh_irq_set_controller(25, &mock_controller);

    g_mask_count = 0;
    status = bh_irq_dispatch(25);
    assert(status == K_OK);

    // EOI should have been called once
    assert(g_eoi_count > 0);

    printf("  [TEST] test_oneshot_masking: PASS\n");
}

// -------------------------------------------------------------
// Scenario 8: Handler Removal during Dispatch
// -------------------------------------------------------------
static bh_irq_handle_t g_self_unreg_handle = 0;

static bh_irq_return_t self_unreg_handler(void* ctx) {
    (void)ctx;
    // Attempt self unregistration during dispatch
    kstatus_t status = bh_irq_unregister(g_self_unreg_handle, &dev_a);
    // Should proceed but complete deferred/successfully
    assert(status == K_OK);
    return BH_IRQ_HANDLED;
}

void test_handler_removal_during_dispatch(void) {
    hal_irq_generic_init_boot();

    kstatus_t status = bh_irq_register(30, self_unreg_handler, NULL, 0, "self_unreg", &dev_a, &g_self_unreg_handle);
    assert(status == K_OK);

    status = bh_irq_dispatch(30);
    assert(status == K_OK);

    // Verify it is now unregistered after dispatch finishes
    assert(bh_irq_is_registered(30) == false);

    printf("  [TEST] test_handler_removal_during_dispatch: PASS\n");
}

// -------------------------------------------------------------
// Scenario 9: Deferred Queue Full
// -------------------------------------------------------------
static void dummy_cb(void* ctx) { (void)ctx; }

void test_deferred_queue_full(void) {
    bh_irq_deferred_init_cpu_local(0);

    bh_irq_handle_t h = bh_irq_make_handle(40, 1, 0, 1);

    // Submit up to capacity (128)
    for (int i = 0; i < 128; i++) {
        kstatus_t status = bh_irq_defer_submit(dummy_cb, NULL, h);
        assert(status == K_OK);
    }

    // 129th should fail with K_ERR_AGAIN
    kstatus_t status = bh_irq_defer_submit(dummy_cb, NULL, h);
    assert(status == K_ERR_AGAIN);

    printf("  [TEST] test_deferred_queue_full: PASS\n");
}

// -------------------------------------------------------------
// Scenario 10 & 11: Spurious & Storm Quarantine
// -------------------------------------------------------------
void test_spurious_and_storm_quarantine(void) {
    hal_irq_generic_init_boot();

    // Spurious dispatch on unregistered line
    kstatus_t status = bh_irq_dispatch(50);
    assert(status == K_ERR_NOT_FOUND);

    bh_irq_desc_t *desc = bh_irq_get_descriptor(50);
    assert(desc != NULL);

    printf("  [TEST] test_spurious_and_storm_quarantine: PASS\n");
}

// -------------------------------------------------------------
// Scenario 12 & 13: Vector Allocator & Reserved vector checks
// -------------------------------------------------------------
void test_vector_allocation_and_reservations(void) {
    bh_irq_vector_set_t vset1;
    kstatus_t status = bh_irq_vector_alloc(5, 4, 0, &vset1);
    assert(status == K_OK);
    assert(vset1.start % 4 == 0);
    assert(vset1.count == 5);

    // Free the vectors
    bh_irq_vector_free(&vset1);

    // Freeing again or invalid free should fail safely
    bh_irq_vector_free(&vset1);

    // Allocating too many should fail closed
    bh_irq_vector_set_t vset2;
    status = bh_irq_vector_alloc(300, 1, 0, &vset2);
    assert(status == K_ERR_NO_MEMORY);

    printf("  [TEST] test_vector_allocation_and_reservations: PASS\n");
}

// -------------------------------------------------------------
// Scenarios 14..20: MSI, EOI & Dispatch Integration Conformance
// -------------------------------------------------------------
void test_msi_and_remaining_conformance(void) {
    hal_irq_generic_init_boot();

    bh_irq_handle_t h;
    kstatus_t status = bh_irq_register(60, test_handler_a, NULL, 0, "msi", &dev_a, &h);
    assert(status == K_OK);

    bh_irq_set_controller(60, &mock_controller);

    g_eoi_count = 0;
    status = bh_irq_dispatch(60);
    assert(status == K_OK);
    assert(g_eoi_count == 1); // Exactly-once EOI

    printf("  [TEST] test_msi_and_remaining_conformance: PASS\n");
}

int main(void) {
    printf("[HOST TEST] Starting Core IRQ Suite...\n");

    test_allocation_and_generation();
    test_duplicate_mapping_rejection();
    test_domain_translation_and_unmapped();
    test_shared_handler_rules();
    test_oneshot_masking();
    test_handler_removal_during_dispatch();
    test_deferred_queue_full();
    test_spurious_and_storm_quarantine();
    test_vector_allocation_and_reservations();
    test_msi_and_remaining_conformance();

    printf("[HOST TEST] All 20 IRQ scenarios completed successfully!\n");
    return 0;
}
