#ifndef BHARAT_INIT_HANDOFF_H
#define BHARAT_INIT_HANDOFF_H

#include "init_profile.h"
#include "init_status.h"
#include "init_manifest.h"

typedef struct {
    uint32_t boot_session_id;
    init_profile_t profile;
    init_phase_t final_phase;
    uint32_t required_started;
    uint32_t required_failed;
    uint32_t optional_started;
    uint32_t optional_failed;
    init_failure_class_t failure_class;
    init_boot_outcome_t outcome;
    uint32_t safe_mode_reason;
} init_handoff_summary_t;

typedef enum {
    INIT_IPC_MSG_INVALID = 0,
    INIT_IPC_MSG_HANDOFF_SUMMARY = 1,
    INIT_IPC_MSG_HANDOFF_ACK = 2,
    INIT_IPC_MSG_BOOT_PHASE_UPDATE = 3,
    INIT_IPC_MSG_SERVICE_STATUS_UPDATE = 4,
} init_ipc_msg_type_t;

typedef struct {
    uint32_t msg_type;
    uint32_t version;
    uint32_t payload_size;
} init_ipc_header_t;

typedef struct {
    init_ipc_header_t hdr;
    init_handoff_summary_t summary;
} init_ipc_handoff_summary_msg_t;

typedef struct {
    init_ipc_header_t hdr;
    uint32_t boot_session_id;
    int status_code;
} init_ipc_handoff_ack_msg_t;

int init_handoff_to_supervisor(const init_boot_context_t *ctx);
int init_send_handoff_to_servicemgr(const init_handoff_summary_t *summary);
int init_send_handoff_to_faultmgr(const init_handoff_summary_t *summary);
bool init_process_handoff_ack(uint32_t boot_session_id, int status_code);

#endif // BHARAT_INIT_HANDOFF_H
