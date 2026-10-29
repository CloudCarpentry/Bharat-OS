#include "kernel.h"
#include "sched/sched.h"
#include "capability.h"
#include "ipc_endpoint.h"
#include "console/console_core.h"
#include "tests/ktest.h"
#include <lib/base/string.h>

#define memcmp __builtin_memcmp

static int test_ipc_envelope_run(void) {
    bh_process_t *proc_sender = process_create("test_ipc_env_sender");
    if (!proc_sender || !proc_sender->security_sandbox_ctx) return -1;

    bh_process_t *proc_recv = process_create("test_ipc_env_receiver");
    if (!proc_recv || !proc_recv->security_sandbox_ctx) return -1;

    capability_table_t *sender_table = (capability_table_t *)proc_sender->security_sandbox_ctx;
    capability_table_t *recv_table = (capability_table_t *)proc_recv->security_sandbox_ctx;

    /* 1. Create communication endpoint in receiver process */
    uint32_t recv_ep_send = 0, recv_ep_recv = 0;
    int res = ipc_endpoint_create(recv_table, &recv_ep_send, &recv_ep_recv);
    if (res != 0 || recv_ep_send == 0 || recv_ep_recv == 0) return -1;

    /* 2. Lookup object ref of communication endpoint to grant send right to sender */
    capability_entry_t entry;
    res = cap_table_lookup(recv_table, recv_ep_recv, CAP_TYPE_ENDPOINT, CAP_RIGHT_ENDPOINT_RECEIVE, &entry);
    if (res != 0) return -1;

    uint32_t sender_send_cap = 0;
    res = cap_table_grant(sender_table, CAP_TYPE_ENDPOINT, entry.object_ref, CAP_RIGHT_ENDPOINT_SEND, &sender_send_cap);
    if (res != 0 || sender_send_cap == 0) return -1;

    /* 3. Create a reply endpoint in sender process */
    uint32_t reply_send = 0, reply_recv = 0;
    res = ipc_endpoint_create(sender_table, &reply_send, &reply_recv);
    if (res != 0 || reply_send == 0 || reply_recv == 0) return -1;

    /* 4. Construct authentic sender provenance */
    ipc_provenance_t prov = {
        .sender_pid = proc_sender->process_id,
        .sender_process_object_id = proc_sender->object_id,
        .sender_cspace_id = sender_table->cspace_id,
        .sender_owner_core = sender_table->owner_core,
        .has_provenance = 1U,
    };

    /* 5. Sender transmits message transferring reply_send endpoint capability */
    const char msg[] = "ENV_TEST";
    res = ipc_endpoint_send_ex(sender_table, sender_send_cap, msg, sizeof(msg), 0U,
                               reply_send, CAP_RIGHT_ENDPOINT_SEND, &prov);
    if (res != IPC_OK) return -1;

    /* 6. Receiver receives message into authenticated V1 receive envelope */
    char recv_buf[32] = {0};
    uint32_t recv_len = 0;
    bh_ipc_receive_envelope_v1_t env = {0};
    res = ipc_endpoint_receive_envelope(recv_table, recv_ep_recv, recv_buf, sizeof(recv_buf),
                                        &recv_len, 0U, &env);
    if (res != IPC_OK) return -1;

    /* 7. Verify message payload */
    if (recv_len != sizeof(msg)) return -1;
    if (memcmp(recv_buf, msg, sizeof(msg)) != 0) return -1;

    /* 8. Verify envelope structure and version */
    if (env.version != BH_IPC_RECEIVE_ENVELOPE_VERSION_V1) return -1;
    if (env.struct_size != sizeof(bh_ipc_receive_envelope_v1_t)) return -1;

    /* 9. Verify kernel-attested sender provenance */
    if (env.sender_pid != proc_sender->process_id) return -1;
    if (env.sender_process_object_id != proc_sender->object_id) return -1;
    if (env.sender_cspace_id != sender_table->cspace_id) return -1;
    if (env.sender_owner_core != sender_table->owner_core) return -1;

    /* 10. Verify validity bitflags */
    if ((env.valid_fields & BH_IPC_ENVELOPE_F_SENDER_PID) == 0) return -1;
    if ((env.valid_fields & BH_IPC_ENVELOPE_F_SENDER_PROCESS_OBJECT) == 0) return -1;
    if ((env.valid_fields & BH_IPC_ENVELOPE_F_SENDER_CSPACE) == 0) return -1;
    if ((env.valid_fields & BH_IPC_ENVELOPE_F_SENDER_OWNER_CORE) == 0) return -1;
    if ((env.valid_fields & BH_IPC_ENVELOPE_F_REPLY_SEND_CAP) == 0) return -1;
    if ((env.valid_fields & BH_IPC_ENVELOPE_F_AUTHORITY_CAP) != 0) return -1;
    if ((env.valid_fields & BH_IPC_ENVELOPE_F_BOOT_SESSION) != 0) return -1;
    if ((env.valid_fields & BH_IPC_ENVELOPE_F_SERVICE_INCARNATION) != 0) return -1;

    /* 11. Verify unprovisioned fields remain zero */
    if (env.boot_session_id != 0) return -1;
    if (env.service_incarnation_id != 0) return -1;
    if (env.authority_cap != 0) return -1;

    /* 12. Verify transferred reply capability */
    if (env.reply_send_cap == 0) return -1;
    res = cap_table_lookup(recv_table, env.reply_send_cap, CAP_TYPE_ENDPOINT, CAP_RIGHT_ENDPOINT_SEND, &entry);
    if (res != 0) return -1;

    /* Clean up installed capability */
    cap_table_revoke(recv_table, env.reply_send_cap);

    console_write_raw("ktest_ipc_envelope_run PASS\n", 28);
    return 0;
}

REGISTER_BOOT_SELFTEST("ipc_authenticated_envelope", "ipc", test_ipc_envelope_run, BOOT_TEST_STAGE_RUNTIME, BOOT_TEST_MANDATORY, 0, false);
