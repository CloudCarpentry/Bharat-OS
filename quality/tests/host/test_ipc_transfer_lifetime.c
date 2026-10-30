/* Real endpoint/CSpace/policy/revocation implementation; only machine,
 * allocation, and scheduler plumbing are substituted. Nonblocking operations
 * deliberately avoid pretending that this harness qualifies guest identity. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ipc_endpoint.h"
#include "../../../core/kernel/src/cap/cap_internal.h"
#include "time/ktime.h"

static uint32_t test_core;
static size_t live_allocations;
static uint64_t test_ticks;
static bh_thread_t scheduler_thread;
static bool simulate_wakes;
static uint32_t block_count;
static capability_table_t *revoke_on_block_table;
static uint32_t revoke_on_block_cap;
static wait_queue_t *last_receiver_queue;
static bh_thread_t *parked_receiver;
static uint32_t wake_count;
static uint32_t queue_initializations;
static bool fail_allocation;
static bool reenter_allocation;
static capability_table_t *reentered_table;
bool g_pmm_initialized = false;
cpu_local_t g_cpu_locals[MAX_CPUS];
uint32_t g_active_core_count = 2U;

void *kmalloc(size_t size) {
        if (reenter_allocation) {
                reenter_allocation = false;
                reentered_table = cap_table_create();
                assert(reentered_table != NULL);
        }
        if (fail_allocation) return NULL;
        void *result = malloc(size);
        if (result != NULL) ++live_allocations;
        return result;
}
void kfree(void *allocation) {
        if (allocation != NULL) --live_allocations;
        free(allocation);
}
uint32_t hal_cpu_get_id(void) { return test_core; }
hal_irq_state_t hal_irq_save_disable(void) { return 0U; }
void hal_irq_restore(hal_irq_state_t state) { (void)state; }
void arch_cpu_relax(void) { ++test_ticks; }
uint64_t sched_get_ticks(void) { return test_ticks; }
bh_thread_t *sched_current_thread(void) {
        return simulate_wakes ? &scheduler_thread : NULL;
}
void sched_block(void) {
        assert(simulate_wakes);
        ++test_ticks;
        ++block_count;
        if (revoke_on_block_table != NULL) {
                assert(cap_table_revoke(revoke_on_block_table, revoke_on_block_cap) == 0);
                revoke_on_block_table = NULL;
        }
}
void sched_wakeup(bh_thread_t *thread) {
        assert(thread != NULL);
        ++wake_count;
}
void sched_wait_queue_init(wait_queue_t *queue) {
        memset(queue, 0, sizeof(*queue));
        if ((++queue_initializations % 2U) == 0U) last_receiver_queue = queue;
}
void sched_wait_queue_enqueue(wait_queue_t *queue, bh_thread_t *thread) {
        (void)queue; (void)thread;
        assert(simulate_wakes);
}
bh_thread_t *sched_wait_queue_dequeue(wait_queue_t *queue) {
        if (queue == last_receiver_queue && parked_receiver != NULL) {
                bh_thread_t *result = parked_receiver;
                parked_receiver = NULL;
                return result;
        }
        return NULL;
}
urpc_channel_state_t urpc_channel_get_state(uint32_t core) {
        (void)core; return URPC_CHANNEL_CLOSED;
}
int urpc_bootstrap_send(uint32_t core, uint64_t payload) {
        (void)core; (void)payload;
        assert(!"owner-local transfer unexpectedly used remote delegation");
        return -1;
}
bh_kdeadline_t bh_deadline_after_ns(uint64_t duration) { return test_ticks + duration; }
bool bh_deadline_expired(bh_kdeadline_t deadline) { return test_ticks >= deadline; }
void kernel_panic(const char *message) { (void)message; abort(); }
void console_log(int level, const char *format, ...) { (void)level; (void)format; }

static capability_table_t *sender_for_endpoint(capability_table_t *receiver,
                                                                                              uint32_t recv_cap,
                                                                                              uint32_t *send_cap,
                                                                                              uint32_t *source_cap) {
        capability_entry_t endpoint;
        assert(cap_table_lookup(receiver, recv_cap, CAP_TYPE_ENDPOINT,
                                                        CAP_RIGHT_ENDPOINT_RECEIVE, &endpoint) == 0);
        capability_table_t *sender = cap_table_create();
        assert(sender != NULL);
        assert(cap_table_grant(sender, CAP_TYPE_ENDPOINT, endpoint.object_ref,
                                                      CAP_RIGHT_ENDPOINT_SEND, send_cap) == 0);
        assert(cap_table_grant(sender, CAP_TYPE_MEMORY, 0x2000U,
                                                      CAP_RIGHT_MEMORY_MAP | CAP_RIGHT_DELEGATE, source_cap) == 0);
        return sender;
    }

static void queue_transfer(capability_table_t *sender, uint32_t send_cap,
                                                      uint32_t source_cap) {
        const uint32_t payload = 0x12345678U;
        assert(ipc_endpoint_send(sender, send_cap, &payload, sizeof(payload), 0U,
                                                          source_cap, CAP_RIGHT_MEMORY_MAP) == IPC_OK);
}
static int receive_transfer(capability_table_t *receiver, uint32_t recv_cap,
                                                        uint32_t *cap) {
        uint32_t payload = 0U, length = 0U;
        int status = ipc_endpoint_receive(receiver, recv_cap, &payload, sizeof(payload),
                                                                            &length, 0U, cap);
        if (status == IPC_OK) {
                assert(length == sizeof(payload) && payload == 0x12345678U);
        } else {
                assert(length == 0U && (cap == NULL || *cap == 0U));
        }
        return status;
}

static void test_repeated_lifecycle(void) {
        capability_table_t *receiver = cap_table_create();
        uint32_t endpoint_send, endpoint_recv;
        assert(receiver != NULL);
        assert(ipc_endpoint_create(receiver, &endpoint_send, &endpoint_recv) == IPC_OK);
        const uint32_t baseline = receiver->id_allocator.allocated_count;
        for (uint32_t iteration = 0U; iteration < 12000U; ++iteration) {
                uint32_t send_cap, source_cap, received = 99U;
                capability_table_t *sender = sender_for_endpoint(receiver, endpoint_recv,
                                                                                                                &send_cap, &source_cap);
                queue_transfer(sender, send_cap, source_cap);
                /* Occupied endpoint cannot overwrite the pending source locator. */
                const uint32_t other = 0U;
                assert(ipc_endpoint_send(sender, send_cap, &other, sizeof(other), 0U,
                                                                  0U, 0U) == IPC_ERR_WOULD_BLOCK);
                switch (iteration % 4U) {
                case 0:
                        assert(receive_transfer(receiver, endpoint_recv, &received) == IPC_OK);
                        assert(cap_table_lookup(receiver, received, CAP_TYPE_MEMORY,
                                                                        CAP_RIGHT_MEMORY_MAP, NULL) == 0);
                        assert(cap_table_lookup(receiver, received, CAP_TYPE_MEMORY,
                                                                        CAP_RIGHT_DELEGATE, NULL) != 0);
                        assert(cap_table_revoke(receiver, received) == 0);
                        break;
                case 1:
                        assert(cap_table_revoke(sender, source_cap) == 0);
                        assert(receive_transfer(receiver, endpoint_recv, &received) ==
                                      IPC_ERR_CAP_INSTALL_FAILED);
                        break;
                case 2:
                        cap_table_destroy(sender);
                        /* Reuse registry slot/storage before consuming the stale locator. */
                        sender = cap_table_create();
                        assert(sender != NULL);
                        assert(cap_table_grant(sender, CAP_TYPE_MEMORY, 0x9000U,
                                                                      CAP_RIGHT_MEMORY_MAP | CAP_RIGHT_DELEGATE,
                                                                      &source_cap) == 0);
                        assert(receive_transfer(receiver, endpoint_recv, &received) ==
                                      IPC_ERR_CAP_INSTALL_FAILED);
                        break;
                default:
                        /* A receiver declining transfer consumes without allocating a cap. */
                        assert(receive_transfer(receiver, endpoint_recv, NULL) == IPC_OK);
                        break;
                }
                cap_table_destroy(sender);
                assert(receiver->id_allocator.allocated_count == baseline);
                assert(live_allocations == 1U);
        }
        cap_table_destroy(receiver);
        assert(live_allocations == 0U);
}

static void test_saturation_and_stale_epoch(void) {
        capability_table_t *receiver = cap_table_create();
        uint32_t endpoint_send, endpoint_recv, send_cap, source_cap, received;
        assert(ipc_endpoint_create(receiver, &endpoint_send, &endpoint_recv) == IPC_OK);
        capability_table_t *sender = sender_for_endpoint(receiver, endpoint_recv,
                                                                                                        &send_cap, &source_cap);
        uint32_t filler[64], count = 0U;
        while (cap_table_grant(receiver, CAP_TYPE_MEMORY, 0x3000U,
                                                      CAP_RIGHT_MEMORY_MAP, &filler[count]) == 0) ++count;
        queue_transfer(sender, send_cap, source_cap);
        received = 99U;
        bh_thread_t waiting = {0};
        parked_receiver = &waiting;
        uint32_t previous_wakes = wake_count;
        assert(receive_transfer(receiver, endpoint_recv, &received) == IPC_ERR_CAP_INSTALL_FAILED);
        assert(parked_receiver == NULL && wake_count == previous_wakes + 1U);
        assert(waiting.ipc_wakeup_reason == IPC_OK && waiting.ipc_deadline_ticks == 0U);
        assert(receiver->id_allocator.allocated_count == 64U);
        assert(cap_table_revoke(receiver, filler[--count]) == 0);
        assert(receive_transfer(receiver, endpoint_recv, &received) == IPC_OK);
        assert(cap_table_revoke(receiver, received) == 0);
        while (count != 0U) assert(cap_table_revoke(receiver, filler[--count]) == 0);

        bh_cap_locator_t locator;
        assert(bh_cap_transfer_capture(sender, source_cap, CAP_RIGHT_MEMORY_MAP, &locator) == K_OK);
        ++locator.revocation_epoch;
        assert(bh_cap_transfer_install(&locator, receiver, CAP_RIGHT_MEMORY_MAP, &received) == K_ERR_CAP_STALE);
        assert(received == 0U);
        assert(bh_cap_transfer_capture(sender, source_cap, CAP_RIGHT_MEMORY_UNMAP, &locator) != K_OK);
        assert(bh_cap_transfer_capture(sender, source_cap, CAP_RIGHT_NONE, &locator) != K_OK);
        cap_table_destroy(sender);
        cap_table_destroy(receiver);
        assert(live_allocations == 0U);
}

static void test_registry_slot64_and_foreign_owner(void) {
        capability_table_t *tables[BH_CAP_CSPACES_PER_CORE];
        for (uint32_t i = 0U; i < BH_CAP_CSPACES_PER_CORE; ++i) {
                tables[i] = cap_table_create();
                assert(tables[i] != NULL);
        }
        uint32_t cap, received;
        assert(cap_table_grant(tables[63], CAP_TYPE_MEMORY, 0x4000U,
                                                      CAP_RIGHT_MEMORY_MAP | CAP_RIGHT_DELEGATE, &cap) == 0);
        bh_cap_locator_t locator;
        assert(bh_cap_transfer_capture(tables[63], cap, CAP_RIGHT_MEMORY_MAP, &locator) == K_OK);
        assert(bh_cap_transfer_install(&locator, tables[0], CAP_RIGHT_MEMORY_MAP, &received) == K_OK);
        assert(cap_table_revoke(tables[0], received) == 0);
        test_core = 1U;
        capability_table_t *foreign = cap_table_create();
        assert(foreign != NULL && foreign->cspace_id != tables[63]->cspace_id);
        assert(bh_cap_transfer_install(&locator, foreign, CAP_RIGHT_MEMORY_MAP, &received) == K_ERR_IPC_CROSS_CORE);
        assert(received == 0U);
        assert(bh_cap_transfer_capture(tables[63], cap, CAP_RIGHT_MEMORY_MAP, &locator) == K_ERR_CAP_OWNERSHIP);
        cap_table_destroy(foreign);
        test_core = 0U;
        for (uint32_t i = 0U; i < BH_CAP_CSPACES_PER_CORE; ++i) cap_table_destroy(tables[i]);
        assert(live_allocations == 0U);
    }

static void test_deadline_and_wakeup_revalidation(void) {
        capability_table_t *receiver = cap_table_create();
        uint32_t endpoint_send, endpoint_recv, send_cap, source_cap;
        assert(ipc_endpoint_create(receiver, &endpoint_send, &endpoint_recv) == IPC_OK);
        capability_table_t *sender = sender_for_endpoint(receiver, endpoint_recv,
                                                                                                        &send_cap, &source_cap);
        uint32_t payload = 0x12345678U, received;
        assert(ipc_endpoint_send(sender, send_cap, &payload, sizeof(payload), 0U, 0U, 0U) == IPC_OK);
        test_ticks = 100U;
        block_count = 0U;
        simulate_wakes = true;
        assert(ipc_endpoint_send(sender, send_cap, &payload, sizeof(payload), 3U, 0U, 0U) == IPC_ERR_TIMEOUT);
        assert(test_ticks == 103U && block_count == 3U);
        test_ticks = UINT64_MAX - 2U;
        block_count = 0U;
        assert(ipc_endpoint_send(sender, send_cap, &payload, sizeof(payload), 10U, 0U, 0U) == IPC_ERR_TIMEOUT);
        assert(test_ticks == UINT64_MAX && block_count == 2U);
        test_ticks = 0U;
        revoke_on_block_table = sender;
        revoke_on_block_cap = source_cap;
        assert(ipc_endpoint_send(sender, send_cap, &payload, sizeof(payload), 3U,
                                                          source_cap, CAP_RIGHT_MEMORY_MAP) == IPC_ERR_CAP_TRANSFER_NOT_ALLOWED);
        assert(receive_transfer(receiver, endpoint_recv, &received) == IPC_OK);
        revoke_on_block_table = receiver;
        revoke_on_block_cap = endpoint_recv;
        uint32_t length = 99U;
        assert(ipc_endpoint_receive(receiver, endpoint_recv, &payload, sizeof(payload),
                                                                &length, 3U, &received) == IPC_ERR_PERM);
        assert(length == 0U && received == 0U);
        simulate_wakes = false;
        cap_table_destroy(sender);
        cap_table_destroy(receiver);
        assert(live_allocations == 0U);
}

static void test_create_reservation_and_allocation_failure(void) {
        reenter_allocation = true;
        capability_table_t *outer = cap_table_create();
        assert(outer != NULL && reentered_table != NULL);
        assert(outer->registry_slot != reentered_table->registry_slot);
        assert(outer->cspace_id != reentered_table->cspace_id);
        cap_table_destroy(reentered_table);
        cap_table_destroy(outer);
        reentered_table = NULL;
        assert(live_allocations == 0U);

        fail_allocation = true;
        capability_table_t *bootstrap = cap_table_create();
        assert(bootstrap != NULL);
        assert(cap_table_create() == NULL);
        for (uint32_t i = 0U; i < BH_CAP_CSPACES_PER_CORE; ++i) {
                assert(!g_cap_cspace_registry[0][i].reserved);
        }
        cap_table_destroy(bootstrap);
        fail_allocation = false;
        outer = cap_table_create();
        assert(outer != NULL);
        cap_table_destroy(outer);
        assert(live_allocations == 0U);
}

static void test_registry_generation_retirement(void) {
        for (uint32_t i = 0U; i < BH_CAP_CSPACES_PER_CORE; ++i) {
                assert(g_cap_cspace_registry[0][i].table == NULL);
                g_cap_cspace_registry[0][i].generation = BH_CAP_CSPACE_GENERATION_MAX;
}
        assert(cap_table_create() == NULL);
        g_cap_cspace_registry[0][0].generation = BH_CAP_CSPACE_GENERATION_MAX - 1U;
        capability_table_t *last = cap_table_create();
        assert(last != NULL && last->registry_slot == 0U);
        assert(g_cap_cspace_registry[0][0].generation == BH_CAP_CSPACE_GENERATION_MAX);
        cap_table_destroy(last);
        assert(cap_table_create() == NULL);
        assert(live_allocations == 0U);
}

static void test_authenticated_envelope_lifecycle(void) {
    capability_table_t *receiver = cap_table_create();
    uint32_t endpoint_send, endpoint_recv;
    assert(receiver != NULL);
    assert(ipc_endpoint_create(receiver, &endpoint_send, &endpoint_recv) == IPC_OK);

    uint32_t send_cap, mem_cap;
    capability_table_t *sender = sender_for_endpoint(receiver, endpoint_recv, &send_cap, &mem_cap);
    assert(sender != NULL);

    ipc_provenance_t prov = {
        .sender_pid = 0xABCD1234ULL,
        .sender_process_object_id = 0x567890ULL,
        .sender_cspace_id = sender->cspace_id,
        .sender_owner_core = sender->owner_core,
        .has_provenance = 1U,
    };

    /* 1. Send with memory capability -> mapped to authority_cap in envelope */
    uint32_t payload = 0xCAFEBABEU;
    assert(ipc_endpoint_send_ex(sender, send_cap, &payload, sizeof(payload), 0U,
                                mem_cap, CAP_RIGHT_MEMORY_MAP, &prov) == IPC_OK);

    bh_ipc_receive_envelope_v1_t env = {0};
    uint32_t received_payload = 0U, received_len = 0U;
    assert(ipc_endpoint_receive_envelope(receiver, endpoint_recv, &received_payload,
                                         sizeof(received_payload), &received_len, 0U, &env) == IPC_OK);
    assert(received_len == sizeof(payload) && received_payload == 0xCAFEBABEU);
    assert(env.version == BH_IPC_RECEIVE_ENVELOPE_VERSION_V1);
    assert(env.struct_size == sizeof(bh_ipc_receive_envelope_v1_t));
    assert((env.valid_fields & BH_IPC_ENVELOPE_F_SENDER_PID) != 0);
    assert((env.valid_fields & BH_IPC_ENVELOPE_F_SENDER_PROCESS_OBJECT) != 0);
    assert((env.valid_fields & BH_IPC_ENVELOPE_F_SENDER_CSPACE) != 0);
    assert((env.valid_fields & BH_IPC_ENVELOPE_F_SENDER_OWNER_CORE) != 0);
    assert((env.valid_fields & BH_IPC_ENVELOPE_F_AUTHORITY_CAP) != 0);
    assert((env.valid_fields & BH_IPC_ENVELOPE_F_REPLY_SEND_CAP) == 0);
    assert((env.valid_fields & BH_IPC_ENVELOPE_F_BOOT_SESSION) == 0);
    assert((env.valid_fields & BH_IPC_ENVELOPE_F_SERVICE_INCARNATION) == 0);
    assert(env.sender_pid == 0xABCD1234ULL);
    assert(env.sender_process_object_id == 0x567890ULL);
    assert(env.sender_cspace_id == sender->cspace_id);
    assert(env.sender_owner_core == sender->owner_core);
    assert(env.authority_cap != 0U);
    assert(env.authority_object_type == CAP_TYPE_MEMORY);
    assert(env.authority_rights == CAP_RIGHT_MEMORY_MAP);
    assert(env.reply_send_cap == 0U);
    assert(env.boot_session_id == 0U);
    assert(env.service_incarnation_id == 0U);
    assert(cap_table_revoke(receiver, env.authority_cap) == 0);

    /* 2. Send with endpoint send capability -> mapped to reply_send_cap */
    uint32_t reply_send, reply_recv;
    assert(ipc_endpoint_create(sender, &reply_send, &reply_recv) == IPC_OK);
    assert(ipc_endpoint_send_ex(sender, send_cap, &payload, sizeof(payload), 0U,
                                reply_send, CAP_RIGHT_ENDPOINT_SEND, &prov) == IPC_OK);

    memset(&env, 0, sizeof(env));
    assert(ipc_endpoint_receive_envelope(receiver, endpoint_recv, &received_payload,
                                         sizeof(received_payload), &received_len, 0U, &env) == IPC_OK);
    assert(env.reply_send_cap != 0U);
    assert(env.authority_cap == 0U);
    assert((env.valid_fields & BH_IPC_ENVELOPE_F_REPLY_SEND_CAP) != 0);
    assert((env.valid_fields & BH_IPC_ENVELOPE_F_AUTHORITY_CAP) == 0);
    assert(cap_table_revoke(receiver, env.reply_send_cap) == 0);

        /* 3. Dual capability transfer (authority_cap + reply_send_cap) */
        prov.reply_cap_to_send = reply_send;
        prov.reply_cap_send_rights = CAP_RIGHT_ENDPOINT_SEND;
        assert(ipc_endpoint_send_ex(sender, send_cap, &payload, sizeof(payload), 0U,
                                                                mem_cap, CAP_RIGHT_MEMORY_MAP, &prov) == IPC_OK);
        memset(&env, 0, sizeof(env));
        assert(ipc_endpoint_receive_envelope(receiver, endpoint_recv, &received_payload,
                                                                                  sizeof(received_payload), &received_len, 0U, &env) == IPC_OK);
        assert(env.authority_cap != 0U);
        assert(env.reply_send_cap != 0U);
        assert((env.valid_fields & BH_IPC_ENVELOPE_F_AUTHORITY_CAP) != 0);
        assert((env.valid_fields & BH_IPC_ENVELOPE_F_REPLY_SEND_CAP) != 0);
        assert(cap_table_revoke(receiver, env.authority_cap) == 0);
        assert(cap_table_revoke(receiver, env.reply_send_cap) == 0);

    cap_table_destroy(sender);
    cap_table_destroy(receiver);
    assert(live_allocations == 0U);
}

static void test_endpoint_close_cancel_and_reclamation(void) {
        for (uint32_t i = 0U; i < BH_CAP_CSPACES_PER_CORE; ++i) {
                g_cap_cspace_registry[0][i].generation = 0U;
        }

        capability_table_t *owner = cap_table_create();
        assert(owner != NULL);

        uint32_t initial_active = ipc_endpoint_active_count_for_core(0U);
        assert(initial_active == 0U);
        for (uint32_t i = 0U; i < 1000U; ++i) {
                uint32_t s_cap = 0U, r_cap = 0U;
                assert(ipc_endpoint_create(owner, &s_cap, &r_cap) == IPC_OK);
                assert(ipc_endpoint_active_count_for_core(0U) == 1U);

                uint32_t s_cap2 = 0U;
                assert(cap_table_delegate(owner, owner, s_cap, CAP_RIGHT_ENDPOINT_SEND, &s_cap2) == 0);
                assert(cap_table_revoke(owner, s_cap2) == 0);

                uint32_t msg = 0x55AA55AAU, out_msg = 0U, out_len = 0U;
                assert(ipc_endpoint_send(owner, s_cap, &msg, sizeof(msg), 0U, 0U, 0U) == IPC_OK);
                assert(ipc_endpoint_receive(owner, r_cap, &out_msg, sizeof(out_msg), &out_len, 0U, NULL) == IPC_OK);
                assert(out_msg == msg);

                assert(ipc_endpoint_owner_close(owner, r_cap) == IPC_OK);
                assert(ipc_endpoint_send(owner, s_cap, &msg, sizeof(msg), 0U, 0U, 0U) == IPC_ERR_CLOSED);

                assert(cap_table_revoke(owner, s_cap) == 0);
                assert(ipc_endpoint_active_count_for_core(0U) == 1U);
                assert(cap_table_revoke(owner, r_cap) == 0);
                assert(ipc_endpoint_active_count_for_core(0U) == 0U);
        }

        uint32_t s_cap = 0U, r_cap = 0U;
        assert(ipc_endpoint_create(owner, &s_cap, &r_cap) == IPC_OK);
        ipc_endpoint_cancel_all_for_thread(&scheduler_thread, IPC_ERR_CANCELLED);
        assert(cap_table_revoke(owner, s_cap) == 0);
        assert(cap_table_revoke(owner, r_cap) == 0);
        assert(ipc_endpoint_active_count_for_core(0U) == 0U);

        cap_table_destroy(owner);
        assert(live_allocations == 0U);
}

int main(void) {
    test_authenticated_envelope_lifecycle();
    test_repeated_lifecycle();
    test_saturation_and_stale_epoch();
    test_registry_slot64_and_foreign_owner();
    test_deadline_and_wakeup_revalidation();
    test_create_reservation_and_allocation_failure();
    test_registry_generation_retirement();
        test_endpoint_close_cancel_and_reclamation();
        puts("PASS: 12000 real endpoint/cap transfer/revoke/teardown cycles; saturation, stale epoch, slot64, foreign owner, close/cancel/reclaim");
    return 0;
}
