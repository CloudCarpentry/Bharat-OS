#include "servicemgr.h"
#include <stddef.h>

// Basic string/mem stub for freestanding tests
void *memset(void *s, int c, size_t n);
char *strncpy(char *dest, const char *src, size_t n);

static service_entry_t service_registry[MAX_SERVICES];
static uint32_t next_service_id = 1;

void servicemgr_init(void) {
    memset(service_registry, 0, sizeof(service_registry));
}

int32_t servicemgr_handle_register(const sm_req_register_t *req, sm_resp_register_t *resp) {
    for (int i = 0; i < MAX_SERVICES; i++) {
        if (!service_registry[i].in_use) {
            service_registry[i].in_use = true;
            service_registry[i].service_id = next_service_id++;
            strncpy(service_registry[i].name, req->service_name, sizeof(service_registry[i].name) - 1);
            service_registry[i].name[sizeof(service_registry[i].name) - 1] = '\0';
            service_registry[i].required_caps = req->required_caps;
            service_registry[i].state = SM_STATE_STOPPED;
            service_registry[i].restart_count = 0;
            service_registry[i].last_heartbeat = 0;

            resp->service_id = service_registry[i].service_id;
            resp->status = BHARAT_IPC_STATUS_OK;
            return BHARAT_IPC_STATUS_OK;
        }
    }
    resp->status = BHARAT_IPC_STATUS_ERR_INTERNAL;
    return BHARAT_IPC_STATUS_ERR_INTERNAL;
}

int32_t servicemgr_handle_start(const sm_req_start_t *req, sm_resp_start_t *resp) {
    for (int i = 0; i < MAX_SERVICES; i++) {
        if (service_registry[i].in_use && service_registry[i].service_id == req->service_id) {
            if (service_registry[i].state == SM_STATE_STOPPED || service_registry[i].state == SM_STATE_CRASHED) {
                service_registry[i].state = SM_STATE_STARTING;
                service_registry[i].last_heartbeat = 0; // Reset heartbeat
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

int32_t servicemgr_handle_stop(const sm_req_stop_t *req, sm_resp_stop_t *resp) {
    for (int i = 0; i < MAX_SERVICES; i++) {
        if (service_registry[i].in_use && service_registry[i].service_id == req->service_id) {
            service_registry[i].state = SM_STATE_STOPPED;
            resp->status = BHARAT_IPC_STATUS_OK;
            return BHARAT_IPC_STATUS_OK;
        }
    }
    resp->status = BHARAT_IPC_STATUS_ERR_NOT_FOUND;
    return BHARAT_IPC_STATUS_ERR_NOT_FOUND;
}

int32_t servicemgr_handle_query(const sm_req_query_t *req, sm_resp_query_t *resp) {
    for (int i = 0; i < MAX_SERVICES; i++) {
        if (service_registry[i].in_use && service_registry[i].service_id == req->service_id) {
            resp->service_id = service_registry[i].service_id;
            resp->state = service_registry[i].state;
            resp->restart_count = service_registry[i].restart_count;
            resp->status = BHARAT_IPC_STATUS_OK;
            return BHARAT_IPC_STATUS_OK;
        }
    }
    resp->status = BHARAT_IPC_STATUS_ERR_NOT_FOUND;
    return BHARAT_IPC_STATUS_ERR_NOT_FOUND;
}

int32_t servicemgr_handle_heartbeat(const sm_req_heartbeat_t *req, sm_resp_heartbeat_t *resp) {
    for (int i = 0; i < MAX_SERVICES; i++) {
        if (service_registry[i].in_use && service_registry[i].service_id == req->service_id) {
            if (service_registry[i].state == SM_STATE_STARTING) {
                service_registry[i].state = SM_STATE_RUNNING;
            }
            if (service_registry[i].state == SM_STATE_RUNNING) {
                service_registry[i].last_heartbeat = req->timestamp;
                resp->status = BHARAT_IPC_STATUS_OK;
                return BHARAT_IPC_STATUS_OK;
            }
            resp->status = BHARAT_IPC_STATUS_ERR_PERM;
            return BHARAT_IPC_STATUS_ERR_PERM;
        }
    }
    resp->status = BHARAT_IPC_STATUS_ERR_NOT_FOUND;
    return BHARAT_IPC_STATUS_ERR_NOT_FOUND;
}

void servicemgr_check_health(uint32_t current_time) {
    for (int i = 0; i < MAX_SERVICES; i++) {
        if (service_registry[i].in_use && service_registry[i].state == SM_STATE_RUNNING) {
            if (current_time > service_registry[i].last_heartbeat + HEARTBEAT_TIMEOUT) {
                // Heartbeat missed -> Crash
                service_registry[i].state = SM_STATE_CRASHED;
                if (service_registry[i].restart_count < MAX_RESTART_COUNT) {
                    service_registry[i].restart_count++;
                    service_registry[i].state = SM_STATE_STARTING;
                } else {
                    service_registry[i].state = SM_STATE_BACKOFF;
                }
            }
        }
    }
}

void servicemgr_loop(bharat_ipc_endpoint_t endpoint) {
    bharat_ipc_msg_header_t req_header;
    bharat_ipc_msg_header_t resp_header;
    uint8_t payload_buf[512];
    uint8_t resp_payload_buf[512];
    uint32_t mock_time = 0;

    while (true) {
        // Assume timeout parameter in a real system to allow health check execution
        int32_t recv_status = bharat_ipc_recv(endpoint, &req_header, payload_buf, sizeof(payload_buf));

        mock_time += 100; // Fake time progression
        servicemgr_check_health(mock_time);

        if (recv_status < 0) {
            continue;
        }

        if (req_header.service_id != SERVICEMGR_SERVICE_ID) {
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
            case SM_OP_REGISTER: {
                if (req_header.payload_size >= sizeof(sm_req_register_t)) {
                    sm_req_register_t *req = (sm_req_register_t*)payload_buf;
                    sm_resp_register_t *resp = (sm_resp_register_t*)resp_payload_buf;
                    dispatch_status = servicemgr_handle_register(req, resp);
                    resp_size = sizeof(sm_resp_register_t);
                } else {
                    dispatch_status = BHARAT_IPC_STATUS_ERR_LENGTH;
                }
                break;
            }
            case SM_OP_START: {
                if (req_header.payload_size >= sizeof(sm_req_start_t)) {
                    sm_req_start_t *req = (sm_req_start_t*)payload_buf;
                    sm_resp_start_t *resp = (sm_resp_start_t*)resp_payload_buf;
                    dispatch_status = servicemgr_handle_start(req, resp);
                    resp_size = sizeof(sm_resp_start_t);
                } else {
                    dispatch_status = BHARAT_IPC_STATUS_ERR_LENGTH;
                }
                break;
            }
            case SM_OP_STOP: {
                if (req_header.payload_size >= sizeof(sm_req_stop_t)) {
                    sm_req_stop_t *req = (sm_req_stop_t*)payload_buf;
                    sm_resp_stop_t *resp = (sm_resp_stop_t*)resp_payload_buf;
                    dispatch_status = servicemgr_handle_stop(req, resp);
                    resp_size = sizeof(sm_resp_stop_t);
                } else {
                    dispatch_status = BHARAT_IPC_STATUS_ERR_LENGTH;
                }
                break;
            }
            case SM_OP_QUERY: {
                if (req_header.payload_size >= sizeof(sm_req_query_t)) {
                    sm_req_query_t *req = (sm_req_query_t*)payload_buf;
                    sm_resp_query_t *resp = (sm_resp_query_t*)resp_payload_buf;
                    dispatch_status = servicemgr_handle_query(req, resp);
                    resp_size = sizeof(sm_resp_query_t);
                } else {
                    dispatch_status = BHARAT_IPC_STATUS_ERR_LENGTH;
                }
                break;
            }
            case SM_OP_HEARTBEAT: {
                if (req_header.payload_size >= sizeof(sm_req_heartbeat_t)) {
                    sm_req_heartbeat_t *req = (sm_req_heartbeat_t*)payload_buf;
                    sm_resp_heartbeat_t *resp = (sm_resp_heartbeat_t*)resp_payload_buf;
                    dispatch_status = servicemgr_handle_heartbeat(req, resp);
                    resp_size = sizeof(sm_resp_heartbeat_t);
                } else {
                    dispatch_status = BHARAT_IPC_STATUS_ERR_LENGTH;
                }
                break;
            }
            default:
                break;
        }

        resp_header.payload_size = resp_size;
        resp_header.flags = dispatch_status;

        if (req_header.reply_endpoint != 0) {
            bharat_ipc_endpoint_t rep_ep = req_header.reply_endpoint;
            bharat_ipc_send(rep_ep, &resp_header, resp_payload_buf);
        }
    }
}
