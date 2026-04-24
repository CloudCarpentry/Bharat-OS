#include "process_manager.h"
#include <stddef.h>
#include <bharat/cap/cap_validate.h>
#include <bharat/uapi/ipc/status.h>

// Basic string/mem stub for freestanding tests
void *memset(void *s, int c, size_t n);
void *memcpy(void *dest, const void *src, size_t n);
char *strncpy(char *dest, const char *src, size_t n);

static process_entry_t process_table[MAX_PROCESSES];
static uint32_t next_pid = 1;

int32_t process_manager_authorize(
    uint32_t opcode,
    const void *req,
    bharat_cap_handle_t caller_cap)
{
    if (caller_cap == BHARAT_CAP_INVALID_HANDLE) {
        return BHARAT_IPC_STATUS_ERR_PERM;
    }

    uint64_t required_rights = 0;
    uint64_t target_object_id = 0;
    bharat_cap_scope_t scope = {
        .kind = BHARAT_CAP_SCOPE_SERVICE,
        .scope_id = 0
    };

    switch (opcode) {
        case PM_OP_CREATE: {
            required_rights = BHARAT_CAP_RIGHT_WRITE;
            break;
        }
        case PM_OP_QUERY: {
            const pm_req_query_t *typed_req = (const pm_req_query_t *)req;
            required_rights = BHARAT_CAP_RIGHT_READ;
            target_object_id = typed_req->process_id;
            scope.kind = BHARAT_CAP_SCOPE_OBJECT;
            scope.scope_id = typed_req->process_id;
            break;
        }
        case PM_OP_START:
        case PM_OP_STOP: {
            const pm_req_start_t *typed_req = (const pm_req_start_t *)req;
            required_rights = BHARAT_CAP_RIGHT_EXECUTE;
            target_object_id = typed_req->process_id;
            scope.kind = BHARAT_CAP_SCOPE_OBJECT;
            scope.scope_id = typed_req->process_id;
            break;
        }
        default:
            return BHARAT_IPC_STATUS_ERR_OPCODE;
    }

    bharat_cap_validation_result_t vr = {0};
    bharat_cap_status_t st = bharat_cap_validate(
        caller_cap,
        BHARAT_CAP_OBJ_PROCESS,
        target_object_id,
        required_rights,
        &scope,
        &vr);

    if (st != BHARAT_CAP_OK || !vr.allowed) {
        return BHARAT_IPC_STATUS_ERR_PERM;
    }

    return BHARAT_IPC_STATUS_OK;
}

void process_manager_init(void) {
    memset(process_table, 0, sizeof(process_table));
}

int32_t process_manager_handle_create(const pm_req_create_t *req, pm_resp_create_t *resp) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (!process_table[i].in_use) {
            process_table[i].in_use = true;
            process_table[i].process_id = next_pid++;
            process_table[i].executable_id = req->executable_id;
            process_table[i].priority = req->priority;
            process_table[i].state = PM_STATE_CREATED;

            resp->process_id = process_table[i].process_id;
            resp->status = BHARAT_IPC_STATUS_OK;
            return BHARAT_IPC_STATUS_OK;
        }
    }
    resp->status = BHARAT_IPC_STATUS_ERR_INTERNAL;
    return BHARAT_IPC_STATUS_ERR_INTERNAL;
}

int32_t process_manager_handle_start(const pm_req_start_t *req, pm_resp_start_t *resp) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].in_use && process_table[i].process_id == req->process_id) {
            if (process_table[i].state == PM_STATE_CREATED || process_table[i].state == PM_STATE_STOPPING) {
                process_table[i].state = PM_STATE_RUNNING;
                resp->status = BHARAT_IPC_STATUS_OK;
                return BHARAT_IPC_STATUS_OK;
            } else {
                resp->status = BHARAT_IPC_STATUS_ERR_PERM;
                return BHARAT_IPC_STATUS_ERR_PERM;
            }
        }
    }
    resp->status = BHARAT_IPC_STATUS_ERR_NOT_FOUND;
    return BHARAT_IPC_STATUS_ERR_NOT_FOUND;
}

int32_t process_manager_handle_stop(const pm_req_stop_t *req, pm_resp_stop_t *resp) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].in_use && process_table[i].process_id == req->process_id) {
            if (process_table[i].state == PM_STATE_RUNNING) {
                process_table[i].state = PM_STATE_STOPPING;
                resp->status = BHARAT_IPC_STATUS_OK;
                return BHARAT_IPC_STATUS_OK;
            } else {
                resp->status = BHARAT_IPC_STATUS_ERR_PERM;
                return BHARAT_IPC_STATUS_ERR_PERM;
            }
        }
    }
    resp->status = BHARAT_IPC_STATUS_ERR_NOT_FOUND;
    return BHARAT_IPC_STATUS_ERR_NOT_FOUND;
}

int32_t process_manager_handle_query(const pm_req_query_t *req, pm_resp_query_t *resp) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].in_use && process_table[i].process_id == req->process_id) {
            resp->process_id = process_table[i].process_id;
            resp->state = process_table[i].state;
            resp->status = BHARAT_IPC_STATUS_OK;
            return BHARAT_IPC_STATUS_OK;
        }
    }
    resp->status = BHARAT_IPC_STATUS_ERR_NOT_FOUND;
    return BHARAT_IPC_STATUS_ERR_NOT_FOUND;
}

void process_manager_loop(bharat_ipc_endpoint_t endpoint) {
    bharat_ipc_msg_header_t req_header;
    bharat_ipc_msg_header_t resp_header;
    uint8_t payload_buf[512];
    uint8_t resp_payload_buf[512];

    while (true) {
        int32_t recv_status = bharat_ipc_recv(endpoint, &req_header, payload_buf, sizeof(payload_buf));
        if (recv_status < 0) {
            continue;
        }

        if (req_header.service_id != PROCESS_MANAGER_SERVICE_ID) {
            continue;
        }

        resp_header.service_id = req_header.service_id;
        resp_header.interface_id = req_header.interface_id;
        resp_header.interface_version = req_header.interface_version;
        resp_header.opcode = req_header.opcode;
        resp_header.message_id = req_header.message_id;

        int32_t dispatch_status = BHARAT_IPC_STATUS_ERR_OPCODE;
        uint32_t resp_size = 0;

        switch (req_header.opcode) {
            case PM_OP_CREATE: {
                if (req_header.payload_size >= sizeof(pm_req_create_t)) {
                    pm_req_create_t *req = (pm_req_create_t*)payload_buf;
                    pm_resp_create_t *resp = (pm_resp_create_t*)resp_payload_buf;
                    int32_t auth_status = process_manager_authorize(req_header.opcode, req, req_header.capability_transfer);
                    if (auth_status != BHARAT_IPC_STATUS_OK) {
                        resp->status = auth_status;
                        dispatch_status = auth_status;
                        resp_size = sizeof(pm_resp_create_t);
                        break;
                    }
                    dispatch_status = process_manager_handle_create(req, resp);
                    resp_size = sizeof(pm_resp_create_t);
                } else {
                    dispatch_status = BHARAT_IPC_STATUS_ERR_LENGTH;
                }
                break;
            }
            case PM_OP_START: {
                if (req_header.payload_size >= sizeof(pm_req_start_t)) {
                    pm_req_start_t *req = (pm_req_start_t*)payload_buf;
                    pm_resp_start_t *resp = (pm_resp_start_t*)resp_payload_buf;
                    int32_t auth_status = process_manager_authorize(req_header.opcode, req, req_header.capability_transfer);
                    if (auth_status != BHARAT_IPC_STATUS_OK) {
                        resp->status = auth_status;
                        dispatch_status = auth_status;
                        resp_size = sizeof(pm_resp_start_t);
                        break;
                    }
                    dispatch_status = process_manager_handle_start(req, resp);
                    resp_size = sizeof(pm_resp_start_t);
                } else {
                    dispatch_status = BHARAT_IPC_STATUS_ERR_LENGTH;
                }
                break;
            }
            case PM_OP_STOP: {
                if (req_header.payload_size >= sizeof(pm_req_stop_t)) {
                    pm_req_stop_t *req = (pm_req_stop_t*)payload_buf;
                    pm_resp_stop_t *resp = (pm_resp_stop_t*)resp_payload_buf;
                    int32_t auth_status = process_manager_authorize(req_header.opcode, req, req_header.capability_transfer);
                    if (auth_status != BHARAT_IPC_STATUS_OK) {
                        resp->status = auth_status;
                        dispatch_status = auth_status;
                        resp_size = sizeof(pm_resp_stop_t);
                        break;
                    }
                    dispatch_status = process_manager_handle_stop(req, resp);
                    resp_size = sizeof(pm_resp_stop_t);
                } else {
                    dispatch_status = BHARAT_IPC_STATUS_ERR_LENGTH;
                }
                break;
            }
            case PM_OP_QUERY: {
                if (req_header.payload_size >= sizeof(pm_req_query_t)) {
                    pm_req_query_t *req = (pm_req_query_t*)payload_buf;
                    pm_resp_query_t *resp = (pm_resp_query_t*)resp_payload_buf;
                    int32_t auth_status = process_manager_authorize(req_header.opcode, req, req_header.capability_transfer);
                    if (auth_status != BHARAT_IPC_STATUS_OK) {
                        resp->status = auth_status;
                        dispatch_status = auth_status;
                        resp_size = sizeof(pm_resp_query_t);
                        break;
                    }
                    dispatch_status = process_manager_handle_query(req, resp);
                    resp_size = sizeof(pm_resp_query_t);
                } else {
                    dispatch_status = BHARAT_IPC_STATUS_ERR_LENGTH;
                }
                break;
            }
            default:
                break;
        }

        resp_header.payload_size = resp_size;

        // Use flags field for status for now if reply fails
        resp_header.flags = dispatch_status;

        if (req_header.reply_endpoint != 0) {
            bharat_ipc_endpoint_t rep_ep = req_header.reply_endpoint;
            bharat_ipc_send(rep_ep, &resp_header, resp_payload_buf);
        }
    }
}
