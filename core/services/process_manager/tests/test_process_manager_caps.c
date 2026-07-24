#include "../process_manager.h"
#include <assert.h>
#include <stdio.h>
#include <bharat/cap/cap_validate.h>
#include <bharat/uapi/ipc/status.h>

int32_t bharat_ipc_send(bharat_ipc_endpoint_t endpoint, const bharat_ipc_msg_header_t *header, const void *payload) { (void)endpoint; (void)header; (void)payload; return 0; }
int32_t bharat_ipc_recv(bharat_ipc_endpoint_t endpoint, bharat_ipc_msg_header_t *header, void *payload_buf, uint32_t max_size) { (void)endpoint; (void)header; (void)payload_buf; (void)max_size; return -1; }

static bharat_cap_status_t fake_backend(
    bharat_cap_handle_t handle,
    bharat_cap_object_type_t expected_object_type,
    uint64_t expected_object_id,
    uint64_t required_rights,
    const bharat_cap_scope_t *required_scope,
    bharat_cap_validation_result_t *out_result)
{
    (void)required_rights;
    if (!out_result) return BHARAT_CAP_INVALID;
    out_result->allowed = false;
    out_result->status = BHARAT_CAP_INVALID;

    if (handle == 100 && expected_object_type == BHARAT_CAP_OBJ_PROCESS) {
        out_result->allowed = true;
        out_result->status = BHARAT_CAP_OK;
        return BHARAT_CAP_OK;
    }

    if (handle == 200) { // replay/stale token
        out_result->status = BHARAT_CAP_STALE;
        return BHARAT_CAP_STALE;
    }

    if (handle == 300 && required_scope && required_scope->kind == BHARAT_CAP_SCOPE_OBJECT && required_scope->scope_id != expected_object_id) {
        out_result->status = BHARAT_CAP_SCOPE_DENIED;
        return BHARAT_CAP_SCOPE_DENIED;
    }

    return BHARAT_CAP_INVALID;
}

int main(void) {
    bharat_cap_set_validate_backend_for_tests(fake_backend);

    pm_req_start_t start_req = {.process_id = 42};
    assert(process_manager_authorize(PM_OP_START, &start_req, BHARAT_CAP_INVALID_HANDLE) == BHARAT_IPC_STATUS_ERR_PERM);
    assert(process_manager_authorize(PM_OP_START, &start_req, 200) == BHARAT_IPC_STATUS_ERR_PERM);
    assert(process_manager_authorize(PM_OP_START, &start_req, 300) == BHARAT_IPC_STATUS_ERR_PERM);
    assert(process_manager_authorize(PM_OP_START, &start_req, 100) == BHARAT_IPC_STATUS_OK);

    puts("process_manager capability negative tests passed");
    return 0;
}
