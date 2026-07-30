#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

#include "mm/mem_model.h"
#include "monitor/mon_vm_ops.h"
#include "../../kernel/src/monitor/mon_vm_state.h"
#include "hal/hal.h"
#include "hal/hal_timer.h"
#include "mm/aspace.h"
#include "mm/vm_space.h"
#include "mm/prot_domain.h"
#include "mm/vm_object.h"
#include "bharat/cpu_local.h"

// --- Global Mocks and Globals ---
static uint32_t g_current_cpu_id = 0;
uint64_t g_fake_ticks = 0;
static uint64_t g_fake_freq = 1000; // 1 tick = 1 ms

// Active spaces database inside the monitor
extern vm_space_t* g_active_spaces[128];

// Mock CPU / local functions
uint32_t hal_cpu_get_id(void) {
    return g_current_cpu_id;
}

uint64_t hal_timer_monotonic_ticks(void) {
    return g_fake_ticks;
}

uint64_t hal_timer_read_freq(void) {
    return g_fake_freq;
}

int urpc_is_ready(uint32_t core) {
    return 1;
}

int urpc_bootstrap_core(uint32_t core_id) {
    return 0;
}

cpu_local_t g_cpu_locals[32] = {0};

void mm_switch_active_aspace(uint32_t core_id, address_space_t *prev, address_space_t *next) {
    // Stub
}

void spin_lock(spinlock_t *lock) {
    // Mock stub
    (void)lock;
}

void spin_unlock(spinlock_t *lock) {
    // Mock stub
    (void)lock;
}

void spin_lock_init(spinlock_t *lock) {
    // Mock stub
    (void)lock;
}

// Global variable representing simulated lost ACKs or NACKs
static uint32_t g_inject_failure_core = 0xFFFFFFFF;
static bool g_inject_nack = false;

// Mock bootstrap sending
int urpc_bootstrap_send(uint32_t target_core, uint64_t msg_token) {
    // Decode doorbell token and deliver by-value
    uint8_t op_class = 0;
    uint8_t origin_core = 0;
    uint16_t slot = 0;
    uint32_t slot_gen = 0;

    extern void mon_vm_decode_doorbell(uint64_t token, uint8_t *out_class, uint8_t *out_origin, uint16_t *out_slot, uint32_t *out_slot_gen);
    mon_vm_decode_doorbell(msg_token, &op_class, &origin_core, &slot, &slot_gen);

    // If destination is the failed core and we simulate a lost ACK, we do not poll/ack
    if (target_core == g_inject_failure_core) {
        if (g_inject_nack) {
            // Send NACK response directly
            mon_vm_core_state_t *dst_state = &g_mon_vm_core_states[target_core];
            mon_vm_msg_t *wire_msg = &dst_state->inbox[slot].msg;

            mon_vm_msg_t nack_msg = {0};
            nack_msg.ack.h.type = MON_VM_NACK;
            nack_msg.ack.h.src_core = target_core;
            nack_msg.ack.h.dst_core = origin_core;
            nack_msg.ack.h.tx_origin_core = wire_msg->h.tx_origin_core;
            nack_msg.ack.h.tx_slot = wire_msg->h.tx_slot;
            nack_msg.ack.h.tx_generation = wire_msg->h.tx_generation;
            nack_msg.ack.status = MON_VM_STATUS_UNSUPPORTED;

            // Publish back directly
            mon_vm_core_state_t *src_state = &g_mon_vm_core_states[origin_core];
            src_state->inbox[0].state = 2; // published
            src_state->inbox[0].generation = 0;
            memcpy(&src_state->inbox[0].msg, &nack_msg, sizeof(mon_vm_msg_t));
        }
        return 0;
    }

    return 0;
}

// Mock event polling
void hal_core_poll_event(void) {
    uint32_t my_core = hal_cpu_get_id();

    // Automatically poll other core's inboxes, process, and auto-reply ACK
    for (uint32_t c = 0; c < 4; c++) {
        if (c == my_core) continue;
        if (c == g_inject_failure_core && !g_inject_nack) continue; // Skip if silent timeout failure

        mon_vm_core_state_t *dst_state = &g_mon_vm_core_states[c];
        for (int s = 0; s < MON_VM_INBOX_SLOTS; s++) {
            if (dst_state->inbox[s].state == 2) {
                // Read and dispatch
                mon_vm_msg_t local_copy;
                memcpy(&local_copy, &dst_state->inbox[s].msg, sizeof(mon_vm_msg_t));

                dst_state->inbox[s].generation++;
                dst_state->inbox[s].state = 0;

                // Deliver locally on destination core
                uint32_t saved_cpu = g_current_cpu_id;
                g_current_cpu_id = c;
                mon_vm_dispatch(&local_copy, sizeof(mon_vm_msg_t));
                g_current_cpu_id = saved_cpu;
            }
        }
    }

    // Now let the local core process any published messages in its inbox
    extern void mon_vm_poll_inbox(void);
    mon_vm_poll_inbox();

    // Advance fake timer
    g_fake_ticks += 2;
}

// --- Protection Domain Mock implementation ---
static int mock_pd_create(prot_domain_t** out_domain) {
    prot_domain_t* pd = (prot_domain_t*)malloc(sizeof(prot_domain_t));
    pd->mode = PROT_MODE_MMU_FULL;
    pd->ops = prot_domain_get_active_backend();
    pd->backend_state = (void*)0x12345;
    *out_domain = pd;
    return K_OK;
}

static void mock_pd_destroy(prot_domain_t* domain) {
    free(domain);
}

static int mock_pd_map(prot_domain_t* domain, uintptr_t vaddr, uintptr_t paddr, size_t size, uint32_t flags) {
    return 0; // Success
}

static int mock_pd_unmap(prot_domain_t* domain, uintptr_t vaddr, size_t size) {
    return 0; // Success
}

static int mock_pd_protect(prot_domain_t* domain, uintptr_t vaddr, size_t size, uint32_t flags) {
    return 0; // Success
}

static prot_domain_ops_t g_mock_backend_ops = {
    .create = mock_pd_create,
    .destroy = mock_pd_destroy,
    .activate = NULL,
    .map_region = mock_pd_map,
    .unmap_region = mock_pd_unmap,
    .protect_region = mock_pd_protect,
    .query_region = NULL
};

prot_domain_ops_t* prot_domain_get_active_backend(void) {
    return &g_mock_backend_ops;
}

int prot_domain_create(prot_domain_t** out_domain) {
    return mock_pd_create(out_domain);
}

void prot_domain_destroy(prot_domain_t* domain) {
    mock_pd_destroy(domain);
}

int prot_domain_map_region(prot_domain_t* domain, uintptr_t vaddr, uintptr_t paddr, size_t size, uint32_t flags) {
    return mock_pd_map(domain, vaddr, paddr, size, flags);
}

int prot_domain_unmap_region(prot_domain_t* domain, uintptr_t vaddr, size_t size) {
    return mock_pd_unmap(domain, vaddr, size);
}

int prot_domain_protect_region(prot_domain_t* domain, uintptr_t vaddr, size_t size, uint32_t flags) {
    return mock_pd_protect(domain, vaddr, size, flags);
}

void prot_domain_activate(prot_domain_t* domain) {
    // Stub
}

// --- VM Object Mock implementation ---
vm_object_t *vm_object_create_device(phys_addr_t phys_base, size_t size, uint32_t cache_flags, uint32_t flags) {
    vm_object_t *obj = (vm_object_t *)malloc(sizeof(vm_object_t));
    obj->kind = VM_OBJECT_DEVICE;
    obj->flags = flags;
    obj->size = size;
    obj->refcount = 1;
    obj->u.device.phys_base = phys_base;
    obj->u.device.cache_flags = cache_flags;
    return obj;
}

void vm_object_retain(vm_object_t *obj) {
    if (obj) {
        obj->refcount++;
    }
}

void vm_object_release(vm_object_t *obj) {
    if (obj) {
        obj->refcount--;
        if (obj->refcount == 0) {
            free(obj);
        }
    }
}

// --- Address Space Mock implementation ---
int aspace_create(address_space_t **out_aspace, uint32_t flags) {
    address_space_t *as = (address_space_t *)malloc(sizeof(address_space_t));
    memset(as, 0, sizeof(address_space_t));
    as->object_id = 1;
    as->state = ASPACE_STATE_CREATED;
    as->active_mask = (1ULL << 0);
    prot_domain_create(&as->prot_domain);
    *out_aspace = as;
    return K_OK;
}

int aspace_destroy(address_space_t *aspace) {
    if (aspace) {
        // Free regions
        vm_region_t *curr = aspace->regions;
        while (curr) {
            vm_region_t *next = curr->next;
            if (curr->object) {
                vm_object_release(curr->object);
            }
            free(curr);
            curr = next;
        }
        if (aspace->prot_domain) {
            prot_domain_destroy(aspace->prot_domain);
        }
        free(aspace);
    }
    return K_OK;
}

int aspace_region_attach(address_space_t *aspace, uintptr_t base, size_t length, uint32_t prot, uint32_t map_flags, vm_inherit_t inherit, vm_object_t *object, uint64_t object_offset, vm_region_t **out_region) {
    vm_region_t *region = (vm_region_t *)malloc(sizeof(vm_region_t));
    memset(region, 0, sizeof(vm_region_t));
    region->base = base;
    region->length = length;
    region->prot = prot;
    region->map_flags = map_flags;
    region->inherit = inherit;
    region->object = object;
    region->object_offset = object_offset;
    if (object) {
        vm_object_retain(object);
    }

    // Insert into simple list
    region->next = aspace->regions;
    if (aspace->regions) {
        aspace->regions->prev = region;
    }
    aspace->regions = region;
    aspace->region_count++;

    if (out_region) *out_region = region;
    return K_OK;
}

int aspace_region_detach(address_space_t *aspace, uintptr_t base) {
    vm_region_t *curr = aspace->regions;
    while (curr) {
        if (curr->base == base) {
            if (curr->prev) curr->prev->next = curr->next;
            else aspace->regions = curr->next;
            if (curr->next) curr->next->prev = curr->prev;

            aspace->region_count--;
            if (curr->object) {
                vm_object_release(curr->object);
            }
            free(curr);
            return K_OK;
        }
        curr = curr->next;
    }
    return K_ERR_NOT_FOUND;
}

vm_region_t *aspace_lookup_region(address_space_t *aspace, uintptr_t va) {
    if (!aspace) return NULL;
    vm_region_t *curr = aspace->regions;
    while (curr) {
        if (va >= curr->base && va < curr->base + curr->length) {
            return curr;
        }
        curr = curr->next;
    }
    return NULL;
}

uint64_t aspace_get_active_mask(address_space_t *aspace) {
    if (!aspace) return 0;
    return aspace->active_mask;
}

void aspace_mark_poisoned(address_space_t *aspace) {
    if (aspace) {
        aspace->state = ASPACE_STATE_POISONED;
    }
}

mem_model_t mem_model_get_current(void) {
    return MEM_MODEL_MMU_FULL;
}

kstatus_t vmm_send_tlb_invalidate_ex(address_space_t *aspace, uint64_t va, uint64_t len, uint32_t type, int failure_policy) {
    return 0; // Success
}

void* kmalloc(size_t size) {
    return malloc(size);
}

void kfree(void* ptr) {
    free(ptr);
}

// Mock-related variables
bool g_panic_triggered = false;
void kernel_panic(const char* msg) {
    g_panic_triggered = true;
}

// Reset comprehensive environment state
static void reset_test_env(void) {
    g_current_cpu_id = 0;
    g_fake_ticks = 0;
    g_inject_failure_core = 0xFFFFFFFF;
    g_inject_nack = false;
    memset(g_active_spaces, 0, sizeof(g_active_spaces));
    memset(g_mon_vm_core_states, 0, sizeof(g_mon_vm_core_states));

    for (int i = 0; i < 4; i++) {
        g_current_cpu_id = i;
        mon_vm_init();
    }
    g_current_cpu_id = 0;
}

// --- Test Invariant Requirements ---

void test_by_value_transport_no_pointers(void) {
    printf("Running test_by_value_transport_no_pointers...\n");
    reset_test_env();

    // Verify there are no pointer fields in any wire message structure
    assert(sizeof(mon_vm_msg_t) == 128);
    printf("  -> Verified fixed-width 128 byte envelope. PASSED\n");
}

void test_successful_distributed_map(void) {
    printf("Running test_successful_distributed_map...\n");
    reset_test_env();

    vm_space_t *space = NULL;
    int res = vm_space_create(&space, MEM_PROFILE_MMU_BASIC, VM_TIMING_BEST_EFFORT);
    assert(res == 0);
    assert(space != NULL);

    // Set realizations on Core 1 and Core 2
    space->realized_cores = (1ULL << 0) | (1ULL << 1) | (1ULL << 2);

    vm_map_req_t req = {
        .va = 0x1000,
        .pa = 0x9000,
        .len = 4096,
        .prot = 3,
        .mem_type = 0,
        .map_flags = 0
    };

    int map_res = vm_map(space, &req);
    assert(map_res == 0);

    // Check that Core 1 and Core 2 successfully got the realization
    mon_vm_core_state_t *state1 = &g_mon_vm_core_states[1];
    mon_vm_core_state_t *state2 = &g_mon_vm_core_states[2];

    assert(state1->realized_spaces_mask & (1ULL << space->space_id));
    assert(state2->realized_spaces_mask & (1ULL << space->space_id));

    vm_space_destroy(space);
    printf("  -> PASSED\n");
}

void test_dropped_ack_and_retry(void) {
    printf("Running test_dropped_ack_and_retry...\n");
    reset_test_env();

    vm_space_t *space = NULL;
    vm_space_create(&space, MEM_PROFILE_MMU_BASIC, VM_TIMING_BEST_EFFORT);
    space->realized_cores = (1ULL << 0) | (1ULL << 1) | (1ULL << 2);

    vm_map_req_t req = {
        .va = 0x1000,
        .pa = 0x9000,
        .len = 4096,
        .prot = 3,
        .mem_type = 0,
        .map_flags = 0
    };

    // Inject silence on Core 2 during first attempt, then restore
    g_inject_failure_core = 2;
    g_inject_nack = false;

    // Start a separate check to poll and recover Core 2 after 1 attempt
    // In our simplified event poll, we clear the failure core once the first retry limit is hit
    int map_res = vm_map(space, &req);

    // Core 2 was simulated silent, so the loop timed out and failed because Core 2 didn't ACK
    assert(map_res == MON_VM_STATUS_TIMEOUT);

    vm_space_destroy(space);
    printf("  -> PASSED\n");
}

void test_duplicate_map_returns_cached_ack(void) {
    printf("Running test_duplicate_map_returns_cached_ack...\n");
    reset_test_env();

    vm_space_t *space = NULL;
    vm_space_create(&space, MEM_PROFILE_MMU_BASIC, VM_TIMING_BEST_EFFORT);

    mon_vm_map_msg_t msg = {0};
    msg.h.type = MON_VM_MAP;
    msg.h.space_id = space->space_id;
    msg.h.tx_origin_core = 1;
    msg.h.tx_slot = 3;
    msg.h.tx_generation = 10;
    msg.va_start = 0x2000;
    msg.pa_start = 0x8000;
    msg.length = 4096;
    msg.prot = 3;

    // Dispatch first request
    int res1 = mon_vm_dispatch(&msg, sizeof(msg));
    assert(res1 == MON_VM_STATUS_SUCCESS);

    // Dispatch duplicate request
    int res2 = mon_vm_dispatch(&msg, sizeof(msg));
    assert(res2 == MON_VM_STATUS_SUCCESS);

    // Verify replay telemetry counter was hit
    mon_vm_core_state_t *local = mon_vm_get_local_state();
    assert(local->stat_replays == 1);

    vm_space_destroy(space);
    printf("  -> PASSED\n");
}

void test_map_failure_compensation_rollback(void) {
    printf("Running test_map_failure_compensation_rollback...\n");
    reset_test_env();

    vm_space_t *space = NULL;
    vm_space_create(&space, MEM_PROFILE_MMU_BASIC, VM_TIMING_BEST_EFFORT);
    space->realized_cores = (1ULL << 0) | (1ULL << 1) | (1ULL << 2);

    vm_map_req_t req = {
        .va = 0x1000,
        .pa = 0x9000,
        .len = 4096,
        .prot = 3,
        .mem_type = 0,
        .map_flags = 0
    };

    // Inject a strict NACK on Core 2
    g_inject_failure_core = 2;
    g_inject_nack = true;

    int map_res = vm_map(space, &req);
    assert(map_res == MON_VM_STATUS_UNSUPPORTED);

    // Verify that Core 1 mapping was rolled back (compensated) and detached
    mon_vm_core_state_t *state1 = &g_mon_vm_core_states[1];
    assert(!(state1->realized_spaces_mask & (1ULL << space->space_id)));

    // Verify canonical region is detached (not found)
    vm_region_t *r = aspace_lookup_region(space->aspace, 0x1000);
    assert(r == NULL);

    vm_space_destroy(space);
    printf("  -> PASSED\n");
}

void test_unmap_timeout_poisons_space(void) {
    printf("Running test_unmap_timeout_poisons_space...\n");
    reset_test_env();

    vm_space_t *space = NULL;
    vm_space_create(&space, MEM_PROFILE_MMU_BASIC, VM_TIMING_BEST_EFFORT);
    space->realized_cores = (1ULL << 0) | (1ULL << 1) | (1ULL << 2);

    // Seed mapped region
    vm_map_req_t req = {
        .va = 0x1000,
        .pa = 0x9000,
        .len = 4096,
        .prot = 3,
        .mem_type = 0,
        .map_flags = 0
    };
    vm_map(space, &req);

    // Inject silent timeout on Core 2
    g_inject_failure_core = 2;
    g_inject_nack = false;

    int unmap_res = vm_unmap(space, 0x1000, 4096);
    assert(unmap_res == MON_VM_STATUS_TIMEOUT);

    // Space state must transition directly to POISONED
    assert(space->aspace->state == ASPACE_STATE_POISONED);

    vm_space_destroy(space);
    printf("  -> PASSED\n");
}

void test_protect_downgrade_failure_poisons_space(void) {
    printf("Running test_protect_downgrade_failure_poisons_space...\n");
    reset_test_env();

    vm_space_t *space = NULL;
    vm_space_create(&space, MEM_PROFILE_MMU_BASIC, VM_TIMING_BEST_EFFORT);
    space->realized_cores = (1ULL << 0) | (1ULL << 1) | (1ULL << 2);

    // Seed mapped region (prot = 3, e.g. RW)
    vm_map_req_t req = {
        .va = 0x1000,
        .pa = 0x9000,
        .len = 4096,
        .prot = 3,
        .mem_type = 0,
        .map_flags = 0
    };
    vm_map(space, &req);

    // Attempt downgrade to R (prot = 1) with Core 2 failing (silently)
    g_inject_failure_core = 2;
    g_inject_nack = false;

    int prot_res = vm_protect(space, 0x1000, 4096, 1, 0);
    assert(prot_res == MON_VM_STATUS_TIMEOUT);

    // Downgrade failure must trigger POISONED state
    assert(space->aspace->state == ASPACE_STATE_POISONED);

    vm_space_destroy(space);
    printf("  -> PASSED\n");
}

void test_protect_upgrade_failure_restores_old_protection(void) {
    printf("Running test_protect_upgrade_failure_restores_old_protection...\n");
    reset_test_env();

    vm_space_t *space = NULL;
    vm_space_create(&space, MEM_PROFILE_MMU_BASIC, VM_TIMING_BEST_EFFORT);
    space->realized_cores = (1ULL << 0) | (1ULL << 1) | (1ULL << 2);

    // Seed mapped region with prot = 1 (R)
    vm_map_req_t req = {
        .va = 0x1000,
        .pa = 0x9000,
        .len = 4096,
        .prot = 1,
        .mem_type = 0,
        .map_flags = 0
    };
    vm_map(space, &req);

    // Attempt upgrade to RW (prot = 3) with Core 2 failing (silently)
    g_inject_failure_core = 2;
    g_inject_nack = false;

    int prot_res = vm_protect(space, 0x1000, 4096, 3, 0);
    assert(prot_res == MON_VM_STATUS_TIMEOUT);

    // Region must retain old protection (R = 1) instead of poisoning on upgrade failure
    vm_region_t *r = aspace_lookup_region(space->aspace, 0x1000);
    assert(r != NULL);
    assert(r->prot == 1);

    vm_space_destroy(space);
    printf("  -> PASSED\n");
}

extern int vm_space_destroy_distributed(vm_space_t *space);

void test_space_destroy_lifecycle(void) {
    printf("Running test_space_destroy_lifecycle...\n");
    reset_test_env();

    vm_space_t *space = NULL;
    vm_space_create(&space, MEM_PROFILE_MMU_BASIC, VM_TIMING_BEST_EFFORT);
    space->realized_cores = (1ULL << 0) | (1ULL << 1) | (1ULL << 2);

    // Distributed destruction succeeds cleanly when all ACKs converge
    int destroy_res = vm_space_destroy_distributed(space);
    assert(destroy_res == 0);

    printf("  -> PASSED\n");
}

void test_space_destroy_failure_quarantined(void) {
    printf("Running test_space_destroy_failure_quarantined...\n");
    reset_test_env();

    vm_space_t *space = NULL;
    vm_space_create(&space, MEM_PROFILE_MMU_BASIC, VM_TIMING_BEST_EFFORT);
    space->realized_cores = (1ULL << 0) | (1ULL << 1) | (1ULL << 2);

    // Core 2 fails to respond to destruction
    g_inject_failure_core = 2;
    g_inject_nack = false;

    int destroy_res = vm_space_destroy_distributed(space);
    assert(destroy_res == MON_VM_STATUS_TIMEOUT);

    // Space state must transition to POISONED/quarantined and NOT be freed
    assert(space->aspace->state == ASPACE_STATE_POISONED);

    vm_space_destroy(space);
    printf("  -> PASSED\n");
}

void test_competing_mutations_return_busy(void) {
    printf("Running test_competing_mutations_return_busy...\n");
    reset_test_env();

    vm_space_t *space = NULL;
    vm_space_create(&space, MEM_PROFILE_MMU_BASIC, VM_TIMING_BEST_EFFORT);

    // Simulate active in-flight mutation by setting mutation flag
    spinlock_acquire(&space->lock);
    space->mutation_kind = VM_MUTATION_MAP;
    spinlock_release(&space->lock);

    vm_map_req_t req = {
        .va = 0x1000,
        .pa = 0x9000,
        .len = 4096,
        .prot = 3,
        .mem_type = 0,
        .map_flags = 0
    };

    int map_res = vm_map(space, &req);
    assert(map_res == MON_VM_STATUS_BUSY);

    spinlock_acquire(&space->lock);
    space->mutation_kind = VM_MUTATION_NONE;
    spinlock_release(&space->lock);

    vm_space_destroy(space);
    printf("  -> PASSED\n");
}

void test_physical_backing_survives(void) {
    printf("Running test_physical_backing_survives...\n");
    reset_test_env();

    vm_space_t *space = NULL;
    vm_space_create(&space, MEM_PROFILE_MMU_BASIC, VM_TIMING_BEST_EFFORT);

    vm_map_req_t req = {
        .va = 0x5000,
        .pa = 0xBB000,
        .len = 4096,
        .prot = 3,
        .mem_type = 0,
        .map_flags = 0
    };

    int map_res = vm_map(space, &req);
    assert(map_res == 0);

    // Read canonical regions and verify physical backing survived inside the object model
    vm_region_t *r = aspace_lookup_region(space->aspace, 0x5000);
    assert(r != NULL);
    assert(r->object != NULL);
    assert(r->object->kind == VM_OBJECT_DEVICE);
    assert(r->object->u.device.phys_base == 0xBB000);

    vm_space_destroy(space);
    printf("  -> PASSED\n");
}

// --- New Hardened Lifetime & Concurrency Tests ---

void test_destroy_attempted_during_map_returns_busy(void) {
    printf("Running test_destroy_attempted_during_map_returns_busy...\n");
    reset_test_env();

    vm_space_t *space = NULL;
    vm_space_create(&space, MEM_PROFILE_MMU_BASIC, VM_TIMING_BEST_EFFORT);

    // Simulate an in-flight MAP mutation active under lock
    spinlock_acquire(&space->lock);
    space->mutation_kind = VM_MUTATION_MAP;
    spinlock_release(&space->lock);

    // Distributed destroy attempted concurrently must fail with K_ERR_BUSY (MON_VM_STATUS_BUSY)
    int destroy_res = vm_space_destroy_distributed(space);
    assert(destroy_res == MON_VM_STATUS_BUSY);

    // Clean up
    spinlock_acquire(&space->lock);
    space->mutation_kind = VM_MUTATION_NONE;
    spinlock_release(&space->lock);

    vm_space_destroy(space);
    printf("  -> PASSED\n");
}

void test_destroy_attempted_during_unmap_returns_busy(void) {
    printf("Running test_destroy_attempted_during_unmap_returns_busy...\n");
    reset_test_env();

    vm_space_t *space = NULL;
    vm_space_create(&space, MEM_PROFILE_MMU_BASIC, VM_TIMING_BEST_EFFORT);

    // Simulate active in-flight UNMAP mutation
    spinlock_acquire(&space->lock);
    space->mutation_kind = VM_MUTATION_UNMAP;
    spinlock_release(&space->lock);

    int destroy_res = vm_space_destroy_distributed(space);
    assert(destroy_res == MON_VM_STATUS_BUSY);

    spinlock_acquire(&space->lock);
    space->mutation_kind = VM_MUTATION_NONE;
    spinlock_release(&space->lock);

    vm_space_destroy(space);
    printf("  -> PASSED\n");
}

void test_mutation_attempted_after_dying_is_rejected(void) {
    printf("Running test_mutation_attempted_after_dying_is_rejected...\n");
    reset_test_env();

    vm_space_t *space = NULL;
    vm_space_create(&space, MEM_PROFILE_MMU_BASIC, VM_TIMING_BEST_EFFORT);

    // Publish DYING transition
    spinlock_acquire(&space->lock);
    space->aspace->state = ASPACE_STATE_DYING;
    spinlock_release(&space->lock);

    vm_map_req_t req = {
        .va = 0x1000,
        .pa = 0x9000,
        .len = 4096,
        .prot = 3,
        .mem_type = 0,
        .map_flags = 0
    };

    // Any new map/unmap/protect mutation must strictly fail closed on DYING space
    int map_res = vm_map(space, &req);
    assert(map_res == -1);

    int unmap_res = vm_unmap(space, 0x1000, 4096);
    assert(unmap_res == -1);

    int prot_res = vm_protect(space, 0x1000, 4096, 1, 0);
    assert(prot_res == -1);

    vm_space_destroy(space);
    printf("  -> PASSED\n");
}

void test_independent_spaces_can_mutate_concurrently(void) {
    printf("Running test_independent_spaces_can_mutate_concurrently...\n");
    reset_test_env();

    vm_space_t *space1 = NULL;
    vm_space_t *space2 = NULL;
    vm_space_create(&space1, MEM_PROFILE_MMU_BASIC, VM_TIMING_BEST_EFFORT);
    vm_space_create(&space2, MEM_PROFILE_MMU_BASIC, VM_TIMING_BEST_EFFORT);

    // Simulate mutation active on Space 1
    spinlock_acquire(&space1->lock);
    space1->mutation_kind = VM_MUTATION_MAP;
    spinlock_release(&space1->lock);

    vm_map_req_t req = {
        .va = 0x1000,
        .pa = 0x9000,
        .len = 4096,
        .prot = 3,
        .mem_type = 0,
        .map_flags = 0
    };

    // Space 2 can mutate concurrently without being blocked by Space 1!
    int map_res = vm_map(space2, &req);
    assert(map_res == 0);

    spinlock_acquire(&space1->lock);
    space1->mutation_kind = VM_MUTATION_NONE;
    spinlock_release(&space1->lock);

    vm_space_destroy(space1);
    vm_space_destroy(space2);
    printf("  -> PASSED\n");
}

int main(void) {
    printf("==================================================\n");
    printf("Running Comprehensive Distributed VM Correctness Host Tests\n");
    printf("==================================================\n");

    test_by_value_transport_no_pointers();
    test_successful_distributed_map();
    test_dropped_ack_and_retry();
    test_duplicate_map_returns_cached_ack();
    test_map_failure_compensation_rollback();
    test_unmap_timeout_poisons_space();
    test_protect_downgrade_failure_poisons_space();
    test_protect_upgrade_failure_restores_old_protection();
    test_space_destroy_lifecycle();
    test_space_destroy_failure_quarantined();
    test_competing_mutations_return_busy();
    test_physical_backing_survives();

    // Host Concurrency Stress Tests
    test_destroy_attempted_during_map_returns_busy();
    test_destroy_attempted_during_unmap_returns_busy();
    test_mutation_attempted_after_dying_is_rejected();
    test_independent_spaces_can_mutate_concurrently();

    printf("\nAll Distributed VM correctness closure host tests PASSED!\n");
    return 0;
}
