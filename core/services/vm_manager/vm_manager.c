#include "vm_manager.h"
#include <stddef.h>
#include <bharat/cap/cap_validate.h>
#include <bharat/uapi/ipc/status.h>

// Basic string/mem stub for freestanding tests
void *memset(void *s, int c, size_t n);

static region_entry_t region_table[MAX_REGIONS];
static uint32_t next_region_id = 1;

static const region_entry_t *vm_manager_find_region(uint32_t region_id) {
    for (int i = 0; i < MAX_REGIONS; i++) {
        if (region_table[i].in_use && region_table[i].region_id == region_id) {
            return &region_table[i];
        }
    }
    return NULL;
}

int32_t vm_manager_authorize(
    uint32_t opcode,
    const void *req,
    bharat_cap_handle_t caller_cap)
{
    if (caller_cap == BHARAT_CAP_INVALID_HANDLE) {
        return BHARAT_IPC_STATUS_ERR_PERM;
    }

    uint64_t required_rights = 0;
    uint64_t target_vm_space = 0;
    bharat_cap_scope_t scope = {
        .kind = BHARAT_CAP_SCOPE_OBJECT,
        .scope_id = 0
    };

    switch (opcode) {
        case VM_OP_MAP: {
            const vm_req_map_t *typed_req = (const vm_req_map_t *)req;
            required_rights = BHARAT_CAP_RIGHT_WRITE;
            target_vm_space = typed_req->aspace_id;
            scope.scope_id = typed_req->aspace_id;
            break;
        }
        case VM_OP_FAULT: {
            const vm_req_fault_t *typed_req = (const vm_req_fault_t *)req;
            required_rights = BHARAT_CAP_RIGHT_WRITE;
            target_vm_space = typed_req->aspace_id;
            scope.scope_id = typed_req->aspace_id;
            break;
        }
        case VM_OP_UNMAP:
        case VM_OP_PROTECT:
        case VM_OP_QUERY: {
            uint32_t region_id = 0;
            if (opcode == VM_OP_UNMAP) {
                const vm_req_unmap_t *typed_req = (const vm_req_unmap_t *)req;
                region_id = typed_req->region_id;
                required_rights = BHARAT_CAP_RIGHT_WRITE;
            } else if (opcode == VM_OP_PROTECT) {
                const vm_req_protect_t *typed_req = (const vm_req_protect_t *)req;
                region_id = typed_req->region_id;
                required_rights = BHARAT_CAP_RIGHT_WRITE;
            } else {
                const vm_req_query_t *typed_req = (const vm_req_query_t *)req;
                region_id = typed_req->region_id;
                required_rights = BHARAT_CAP_RIGHT_READ;
            }

            const region_entry_t *region = vm_manager_find_region(region_id);
            if (!region) {
                return BHARAT_IPC_STATUS_ERR_NOT_FOUND;
            }
            target_vm_space = region->aspace_id;
            scope.scope_id = region->aspace_id;
            break;
        }
        default:
            return BHARAT_IPC_STATUS_ERR_OPCODE;
    }

    bharat_cap_validation_result_t vr = {0};
    bharat_cap_status_t st = bharat_cap_validate(
        caller_cap,
        BHARAT_CAP_OBJ_VM_SPACE,
        target_vm_space,
        required_rights,
        &scope,
        &vr);

    if (st != BHARAT_CAP_OK || !vr.allowed) {
        return BHARAT_IPC_STATUS_ERR_PERM;
    }

    return BHARAT_IPC_STATUS_OK;
}

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
                    int32_t auth_status = vm_manager_authorize(req_header.opcode, req, req_header.capability_transfer);
                    if (auth_status != BHARAT_IPC_STATUS_OK) {
                        resp->status = auth_status;
                        dispatch_status = auth_status;
                        resp_size = sizeof(vm_resp_map_t);
                        break;
                    }
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
                    int32_t auth_status = vm_manager_authorize(req_header.opcode, req, req_header.capability_transfer);
                    if (auth_status != BHARAT_IPC_STATUS_OK) {
                        resp->status = auth_status;
                        dispatch_status = auth_status;
                        resp_size = sizeof(vm_resp_unmap_t);
                        break;
                    }
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
                    int32_t auth_status = vm_manager_authorize(req_header.opcode, req, req_header.capability_transfer);
                    if (auth_status != BHARAT_IPC_STATUS_OK) {
                        resp->status = auth_status;
                        dispatch_status = auth_status;
                        resp_size = sizeof(vm_resp_protect_t);
                        break;
                    }
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
                    int32_t auth_status = vm_manager_authorize(req_header.opcode, req, req_header.capability_transfer);
                    if (auth_status != BHARAT_IPC_STATUS_OK) {
                        resp->status = auth_status;
                        dispatch_status = auth_status;
                        resp_size = sizeof(vm_resp_query_t);
                        break;
                    }
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
                    int32_t auth_status = vm_manager_authorize(req_header.opcode, req, req_header.capability_transfer);
                    if (auth_status != BHARAT_IPC_STATUS_OK) {
                        resp->action = -1;
                        dispatch_status = auth_status;
                        resp_size = sizeof(vm_resp_fault_t);
                        break;
                    }
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
