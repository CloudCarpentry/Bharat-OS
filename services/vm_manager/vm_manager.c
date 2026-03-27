#include "vm_manager.h"
#include <stddef.h>

// Basic string/mem stub for freestanding tests
void *memset(void *s, int c, size_t n);

static region_entry_t region_table[MAX_REGIONS];
static uint32_t next_region_id = 1;

void vm_manager_init(void) {
    memset(region_table, 0, sizeof(region_table));
}

int32_t vm_manager_handle_map(const vm_req_map_t *req, vm_resp_map_t *resp) {
    for (int i = 0; i < MAX_REGIONS; i++) {
        if (!region_table[i].in_use) {
            region_table[i].in_use = true;
            region_table[i].region_id = next_region_id++;
            region_table[i].aspace_id = req->aspace_id;
            region_table[i].vaddr = req->vaddr;
            region_table[i].size = req->size;
            region_table[i].flags = req->flags;
            region_table[i].state = VM_REGION_DECLARED;

            resp->region_id = region_table[i].region_id;
            resp->status = BHARAT_IPC_STATUS_OK;
            return BHARAT_IPC_STATUS_OK;
        }
    }
    resp->status = BHARAT_IPC_STATUS_ERR_INTERNAL;
    return BHARAT_IPC_STATUS_ERR_INTERNAL;
}

int32_t vm_manager_handle_unmap(const vm_req_unmap_t *req, vm_resp_unmap_t *resp) {
    for (int i = 0; i < MAX_REGIONS; i++) {
        if (region_table[i].in_use && region_table[i].region_id == req->region_id) {
            region_table[i].in_use = false;
            region_table[i].state = VM_REGION_REVOKED;
            resp->status = BHARAT_IPC_STATUS_OK;
            return BHARAT_IPC_STATUS_OK;
        }
    }
    resp->status = BHARAT_IPC_STATUS_ERR_NOT_FOUND;
    return BHARAT_IPC_STATUS_ERR_NOT_FOUND;
}

int32_t vm_manager_handle_protect(const vm_req_protect_t *req, vm_resp_protect_t *resp) {
    for (int i = 0; i < MAX_REGIONS; i++) {
        if (region_table[i].in_use && region_table[i].region_id == req->region_id) {
            region_table[i].flags = req->new_flags;
            region_table[i].state = VM_REGION_VALIDATED;
            resp->status = BHARAT_IPC_STATUS_OK;
            return BHARAT_IPC_STATUS_OK;
        }
    }
    resp->status = BHARAT_IPC_STATUS_ERR_NOT_FOUND;
    return BHARAT_IPC_STATUS_ERR_NOT_FOUND;
}

int32_t vm_manager_handle_query(const vm_req_query_t *req, vm_resp_query_t *resp) {
    for (int i = 0; i < MAX_REGIONS; i++) {
        if (region_table[i].region_id == req->region_id) {
            resp->region_id = region_table[i].region_id;
            resp->state = region_table[i].state;
            resp->vaddr = region_table[i].vaddr;
            resp->size = region_table[i].size;
            resp->status = BHARAT_IPC_STATUS_OK;
            return BHARAT_IPC_STATUS_OK;
        }
    }
    resp->status = BHARAT_IPC_STATUS_ERR_NOT_FOUND;
    return BHARAT_IPC_STATUS_ERR_NOT_FOUND;
}

int32_t vm_manager_handle_fault(const vm_req_fault_t *req, vm_resp_fault_t *resp) {
    for (int i = 0; i < MAX_REGIONS; i++) {
        if (region_table[i].in_use && region_table[i].aspace_id == req->aspace_id) {
            if (req->fault_vaddr >= region_table[i].vaddr && req->fault_vaddr < (region_table[i].vaddr + region_table[i].size)) {
                region_table[i].state = VM_REGION_ACTIVE;
                resp->action = 0; // Resolved
                return BHARAT_IPC_STATUS_OK;
            }
        }
    }
    resp->action = -1; // Kill
    return BHARAT_IPC_STATUS_ERR_NOT_FOUND;
}

void vm_manager_loop(bharat_ipc_endpoint_t endpoint) {
    bharat_ipc_msg_header_t req_header;
    bharat_ipc_msg_header_t resp_header;
    uint8_t payload_buf[512];
    uint8_t resp_payload_buf[512];

    while (true) {
        int32_t recv_status = bharat_ipc_recv(endpoint, &req_header, payload_buf, sizeof(payload_buf));
        if (recv_status < 0) {
            continue;
        }

        if (req_header.service_id != VM_MANAGER_SERVICE_ID) {
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
            case VM_OP_MAP: {
                if (req_header.payload_size >= sizeof(vm_req_map_t)) {
                    vm_req_map_t *req = (vm_req_map_t*)payload_buf;
                    vm_resp_map_t *resp = (vm_resp_map_t*)resp_payload_buf;
                    dispatch_status = vm_manager_handle_map(req, resp);
                    resp_size = sizeof(vm_resp_map_t);
                } else {
                    dispatch_status = BHARAT_IPC_STATUS_ERR_LENGTH;
                }
                break;
            }
            case VM_OP_UNMAP: {
                if (req_header.payload_size >= sizeof(vm_req_unmap_t)) {
                    vm_req_unmap_t *req = (vm_req_unmap_t*)payload_buf;
                    vm_resp_unmap_t *resp = (vm_resp_unmap_t*)resp_payload_buf;
                    dispatch_status = vm_manager_handle_unmap(req, resp);
                    resp_size = sizeof(vm_resp_unmap_t);
                } else {
                    dispatch_status = BHARAT_IPC_STATUS_ERR_LENGTH;
                }
                break;
            }
            case VM_OP_PROTECT: {
                if (req_header.payload_size >= sizeof(vm_req_protect_t)) {
                    vm_req_protect_t *req = (vm_req_protect_t*)payload_buf;
                    vm_resp_protect_t *resp = (vm_resp_protect_t*)resp_payload_buf;
                    dispatch_status = vm_manager_handle_protect(req, resp);
                    resp_size = sizeof(vm_resp_protect_t);
                } else {
                    dispatch_status = BHARAT_IPC_STATUS_ERR_LENGTH;
                }
                break;
            }
            case VM_OP_QUERY: {
                if (req_header.payload_size >= sizeof(vm_req_query_t)) {
                    vm_req_query_t *req = (vm_req_query_t*)payload_buf;
                    vm_resp_query_t *resp = (vm_resp_query_t*)resp_payload_buf;
                    dispatch_status = vm_manager_handle_query(req, resp);
                    resp_size = sizeof(vm_resp_query_t);
                } else {
                    dispatch_status = BHARAT_IPC_STATUS_ERR_LENGTH;
                }
                break;
            }
            case VM_OP_FAULT: {
                if (req_header.payload_size >= sizeof(vm_req_fault_t)) {
                    vm_req_fault_t *req = (vm_req_fault_t*)payload_buf;
                    vm_resp_fault_t *resp = (vm_resp_fault_t*)resp_payload_buf;
                    dispatch_status = vm_manager_handle_fault(req, resp);
                    resp_size = sizeof(vm_resp_fault_t);
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
