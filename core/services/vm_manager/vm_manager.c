#include "vm_manager.h"
#include <stddef.h>
#include <bharat/cap/cap_validate.h>
#include <bharat/uapi/ipc/status.h>

// Handle library
#include <handle_table.h>

// Custom basic mem/string implementations to ensure freestanding compilation
static void *local_memset(void *s, int c, size_t n) {
    unsigned char *p = (unsigned char *)s;
    while (n--) {
        *p++ = (unsigned char)c;
    }
    return s;
}

static region_entry_t region_table[MAX_REGIONS];
static uint32_t next_region_id = 1;

// v1 Storage and Handles
static bh_user_handle_table_t g_vm_space_handle_table;
static bh_user_handle_slot_t g_vm_space_slots[MAX_SPACES];
static bh_vm_space_v1_t g_vm_spaces[MAX_SPACES];

static bh_user_handle_table_t g_vm_region_handle_table;
static bh_user_handle_slot_t g_vm_region_slots[MAX_REGIONS];
static bh_vm_region_v1_t g_vm_regions[MAX_REGIONS];

static const region_entry_t *vm_manager_find_region(uint32_t region_id) {
    for (int i = 0; i < MAX_REGIONS; i++) {
        if (region_table[i].in_use && region_table[i].region_id == region_id) {
            return &region_table[i];
        }
    }
    return NULL;
}

// Default authority ops
static int32_t default_space_create(void *ctx, const bh_vm_create_space_request_v1_t *req, bh_vm_kernel_space_ref_t *out_ref) {
    (void)ctx; (void)req;
    out_ref->space_id = 9911;
    return 0;
}

static int32_t default_space_destroy(void *ctx, bh_vm_kernel_space_ref_t ref) {
    (void)ctx; (void)ref;
    return 0;
}

static int32_t default_map(void *ctx, bh_vm_kernel_space_ref_t ref, const bh_vm_map_request_v1_t *req) {
    (void)ctx; (void)ref; (void)req;
    return 0;
}

static int32_t default_unmap(void *ctx, bh_vm_kernel_space_ref_t ref, uint64_t vaddr, uint64_t length) {
    (void)ctx; (void)ref; (void)vaddr; (void)length;
    return 0;
}

static int32_t default_protect(void *ctx, bh_vm_kernel_space_ref_t ref, uint64_t vaddr, uint64_t length, uint64_t protection, uint64_t memory_type) {
    (void)ctx; (void)ref; (void)vaddr; (void)length; (void)protection; (void)memory_type;
    return 0;
}

static int32_t default_query(void *ctx, bh_vm_kernel_space_ref_t ref, uint64_t vaddr, bh_vm_kernel_query_result_t *out_res) {
    (void)ctx; (void)ref; (void)vaddr;
    out_res->state = 2; // mapped
    out_res->vaddr = vaddr;
    out_res->size = 4096;
    out_res->protection = 3;
    out_res->memory_type = 0;
    return 0;
}

static bh_vm_authority_ops_t g_authority_ops = {
    .ctx = NULL,
    .space_create = default_space_create,
    .space_destroy = default_space_destroy,
    .map = default_map,
    .unmap = default_unmap,
    .protect = default_protect,
    .query = default_query
};

void bh_vm_set_authority_ops(const bh_vm_authority_ops_t *ops) {
    if (ops) {
        g_authority_ops = *ops;
    }
}

int bh_vm_get_active_spaces_count(void) {
    return (int)g_vm_space_handle_table.count;
}

int bh_vm_get_active_regions_count(void) {
    return (int)g_vm_region_handle_table.count;
}

#include <bharat/cap/cap_authz.h>

static const bharat_service_authz_desc_t vm_manager_authz_descs[] = {
    {
        .opcode = VM_OP_MAP,
        .object_type = BHARAT_CAP_OBJ_VM_SPACE,
        .required_rights = BHARAT_CAP_RIGHT_WRITE,
        .required_feature_cap = BHARAT_MEM_CAP_PAGE_MAP,
    },
    {
        .opcode = VM_OP_FAULT,
        .object_type = BHARAT_CAP_OBJ_VM_SPACE,
        .required_rights = BHARAT_CAP_RIGHT_WRITE,
        .required_feature_cap = BHARAT_MEM_CAP_DEMAND_FAULT,
    },
    {
        .opcode = VM_OP_UNMAP,
        .object_type = BHARAT_CAP_OBJ_VM_SPACE,
        .required_rights = BHARAT_CAP_RIGHT_WRITE,
        .required_feature_cap = BHARAT_MEM_CAP_PAGE_MAP,
    },
    {
        .opcode = VM_OP_PROTECT,
        .object_type = BHARAT_CAP_OBJ_VM_SPACE,
        .required_rights = BHARAT_CAP_RIGHT_WRITE,
        .required_feature_cap = BHARAT_MEM_CAP_PAGE_PROTECT,
    },
    {
        .opcode = VM_OP_QUERY,
        .object_type = BHARAT_CAP_OBJ_VM_SPACE,
        .required_rights = BHARAT_CAP_RIGHT_READ,
        .required_feature_cap = BHARAT_MEM_CAP_PAGE_MAP,
    }
};

int32_t vm_manager_authorize(
    uint32_t opcode,
    const void *req,
    bharat_cap_handle_t caller_cap)
{
    if (caller_cap == BHARAT_CAP_INVALID_HANDLE) {
        return BHARAT_IPC_STATUS_ERR_PERM;
    }

    uint64_t target_vm_space = 0;

    switch (opcode) {
        case VM_OP_MAP: {
            const vm_req_map_t *typed_req = (const vm_req_map_t *)req;
            target_vm_space = typed_req->aspace_id;
            break;
        }
        case VM_OP_FAULT: {
            const vm_req_fault_t *typed_req = (const vm_req_fault_t *)req;
            target_vm_space = typed_req->aspace_id;
            break;
        }
        case VM_OP_UNMAP:
        case VM_OP_PROTECT:
        case VM_OP_QUERY: {
            uint32_t region_id = 0;
            if (opcode == VM_OP_UNMAP) {
                const vm_req_unmap_t *typed_req = (const vm_req_unmap_t *)req;
                region_id = typed_req->region_id;
            } else if (opcode == VM_OP_PROTECT) {
                const vm_req_protect_t *typed_req = (const vm_req_protect_t *)req;
                region_id = typed_req->region_id;
            } else {
                const vm_req_query_t *typed_req = (const vm_req_query_t *)req;
                region_id = typed_req->region_id;
            }

            const region_entry_t *region = vm_manager_find_region(region_id);
            if (!region) {
                return BHARAT_IPC_STATUS_ERR_NOT_FOUND;
            }
            target_vm_space = region->aspace_id;
            break;
        }
        default:
            return BHARAT_IPC_STATUS_ERR_OPCODE;
    }

    return bharat_service_dispatch_authorize(
        VM_MANAGER_SERVICE_ID,
        opcode,
        vm_manager_authz_descs,
        sizeof(vm_manager_authz_descs) / sizeof(vm_manager_authz_descs[0]),
        caller_cap,
        target_vm_space
    );
}

void vm_manager_init(void) {
    local_memset(region_table, 0, sizeof(region_table));
    local_memset(g_vm_spaces, 0, sizeof(g_vm_spaces));
    local_memset(g_vm_regions, 0, sizeof(g_vm_regions));

    bh_user_handle_table_init(&g_vm_space_handle_table, g_vm_space_slots, MAX_SPACES);
    bh_user_handle_table_init(&g_vm_region_handle_table, g_vm_region_slots, MAX_REGIONS);
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
    // For v0, fallback to metadata checks
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

// -----------------------------------------------------------------------------
// v1 Interfaces Implementation
// -----------------------------------------------------------------------------

int32_t bh_vm_handle_create_space_v1(const bh_vm_create_space_request_v1_t *req, bh_vm_create_space_response_v1_t *resp) {
    if (!req || !resp) {
        return BHARAT_IPC_STATUS_ERR_INVALID;
    }

    local_memset(resp, 0, sizeof(bh_vm_create_space_response_v1_t));
    resp->abi_version = BH_VM_INTERFACE_VERSION_V1;
    resp->struct_size = sizeof(bh_vm_create_space_response_v1_t);

    if (req->abi_version != BH_VM_INTERFACE_VERSION_V1 || req->struct_size != sizeof(bh_vm_create_space_request_v1_t)) {
        resp->status = BHARAT_IPC_STATUS_ERR_INVALID;
        return BHARAT_IPC_STATUS_ERR_INVALID;
    }

    // Allocate Space Record
    bh_vm_space_v1_t *space = NULL;
    for (int i = 0; i < MAX_SPACES; i++) {
        if (!g_vm_spaces[i].in_use) {
            space = &g_vm_spaces[i];
            break;
        }
    }

    if (!space) {
        resp->status = BHARAT_IPC_STATUS_ERR_LENGTH;
        return BHARAT_IPC_STATUS_ERR_LENGTH;
    }

    bh_vm_kernel_space_ref_t k_ref;
    int k_res = g_authority_ops.space_create(g_authority_ops.ctx, req, &k_ref);
    if (k_res != 0) {
        resp->status = BHARAT_IPC_STATUS_ERR_INTERNAL;
        return BHARAT_IPC_STATUS_ERR_INTERNAL;
    }

    space->in_use = true;
    space->kernel_space_id = k_ref.space_id;
    space->memory_profile = req->memory_profile;
    space->timing_class = req->timing_class;

    bh_handle_t h_space = 0;
    int h_status = bh_user_handle_alloc(&g_vm_space_handle_table, space, BHARAT_CAP_OBJ_VM_SPACE, BHARAT_CAP_RIGHT_WRITE | BHARAT_CAP_RIGHT_READ, &h_space);
    if (h_status != BH_HANDLE_TABLE_SUCCESS) {
        g_authority_ops.space_destroy(g_authority_ops.ctx, k_ref);
        space->in_use = false;
        resp->status = BHARAT_IPC_STATUS_ERR_LENGTH;
        return BHARAT_IPC_STATUS_ERR_LENGTH;
    }

    space->public_handle = h_space;

    resp->status = BHARAT_IPC_STATUS_OK;
    resp->vm_space_handle = h_space;
    return BHARAT_IPC_STATUS_OK;
}

int32_t bh_vm_handle_destroy_space_v1(const bh_vm_destroy_space_request_v1_t *req, bh_vm_destroy_space_response_v1_t *resp) {
    if (!req || !resp) {
        return BHARAT_IPC_STATUS_ERR_INVALID;
    }

    local_memset(resp, 0, sizeof(bh_vm_destroy_space_response_v1_t));
    resp->abi_version = BH_VM_INTERFACE_VERSION_V1;
    resp->struct_size = sizeof(bh_vm_destroy_space_response_v1_t);

    bh_vm_space_v1_t *space = NULL;
    int h_res = bh_user_handle_lookup(&g_vm_space_handle_table, req->vm_space_handle, BHARAT_CAP_OBJ_VM_SPACE, (void **)&space, NULL);
    if (h_res != BH_HANDLE_TABLE_SUCCESS || !space) {
        resp->status = BHARAT_IPC_STATUS_ERR_NOT_FOUND;
        return BHARAT_IPC_STATUS_ERR_NOT_FOUND;
    }

    // Unmap all region mapping records associated with this VM space first
    for (int i = 0; i < MAX_REGIONS; i++) {
        if (g_vm_regions[i].in_use && g_vm_regions[i].parent_space == req->vm_space_handle) {
            bh_vm_kernel_space_ref_t k_space = { .space_id = space->kernel_space_id };
            g_authority_ops.unmap(g_authority_ops.ctx, k_space, g_vm_regions[i].vaddr, g_vm_regions[i].length);
            bh_user_handle_revoke(&g_vm_region_handle_table, g_vm_regions[i].public_handle);
            local_memset(&g_vm_regions[i], 0, sizeof(bh_vm_region_v1_t));
        }
    }

    bh_vm_kernel_space_ref_t k_space = { .space_id = space->kernel_space_id };
    g_authority_ops.space_destroy(g_authority_ops.ctx, k_space);

    bh_user_handle_revoke(&g_vm_space_handle_table, req->vm_space_handle);
    local_memset(space, 0, sizeof(bh_vm_space_v1_t));

    resp->status = BHARAT_IPC_STATUS_OK;
    return BHARAT_IPC_STATUS_OK;
}

int32_t bh_vm_handle_map_v1(const bh_vm_map_request_v1_t *req, bh_vm_map_response_v1_t *resp) {
    if (!req || !resp) {
        return BHARAT_IPC_STATUS_ERR_INVALID;
    }

    local_memset(resp, 0, sizeof(bh_vm_map_response_v1_t));
    resp->abi_version = BH_VM_INTERFACE_VERSION_V1;
    resp->struct_size = sizeof(bh_vm_map_response_v1_t);

    if (req->abi_version != BH_VM_INTERFACE_VERSION_V1 || req->struct_size != sizeof(bh_vm_map_request_v1_t)) {
        resp->status = BHARAT_IPC_STATUS_ERR_INVALID;
        return BHARAT_IPC_STATUS_ERR_INVALID;
    }

    // Zero-size validation
    if (req->size == 0) {
        resp->status = BHARAT_IPC_STATUS_ERR_INVALID;
        return BHARAT_IPC_STATUS_ERR_INVALID;
    }

    // Alignment validation (page size 4KB)
    if ((req->vaddr % 4096 != 0) || (req->size % 4096 != 0)) {
        resp->status = BHARAT_IPC_STATUS_ERR_INVALID;
        return BHARAT_IPC_STATUS_ERR_INVALID;
    }

    // Overflow validation
    if (req->vaddr + req->size < req->vaddr) {
        resp->status = BHARAT_IPC_STATUS_ERR_INVALID;
        return BHARAT_IPC_STATUS_ERR_INVALID;
    }

    // Canonical virtual address bounds check
    if (req->vaddr < 0x1000 || (req->vaddr + req->size) > 0x00007FFFFFFFF000) {
        resp->status = BHARAT_IPC_STATUS_ERR_INVALID;
        return BHARAT_IPC_STATUS_ERR_INVALID;
    }

    // W^X check (2 is write, 4 is execute)
    if ((req->protection & 2) && (req->protection & 4)) {
        resp->status = BHARAT_IPC_STATUS_ERR_INVALID;
        return BHARAT_IPC_STATUS_ERR_INVALID;
    }

    // Lookup space
    bh_vm_space_v1_t *space = NULL;
    int h_res = bh_user_handle_lookup(&g_vm_space_handle_table, req->vm_space_handle, BHARAT_CAP_OBJ_VM_SPACE, (void **)&space, NULL);
    if (h_res != BH_HANDLE_TABLE_SUCCESS || !space) {
        resp->status = BHARAT_IPC_STATUS_ERR_NOT_FOUND;
        return BHARAT_IPC_STATUS_ERR_NOT_FOUND;
    }

    // Unsupported demand paging under constrained profiles (e.g. MPU profile)
    if (space->memory_profile == 0) { // MEM_PROFILE_MPU_ONLY
        // Dynamic map on MPU may be unsupported if timing is hard RT
        if (space->timing_class == 4) { // VM_TIMING_HARD_RT
            resp->status = BHARAT_IPC_STATUS_ERR_INVALID; // UNSUPPORTED
            return BHARAT_IPC_STATUS_ERR_INVALID;
        }
    }

    // Overlap validation
    for (int i = 0; i < MAX_REGIONS; i++) {
        if (g_vm_regions[i].in_use && g_vm_regions[i].parent_space == req->vm_space_handle) {
            uint64_t existing_start = g_vm_regions[i].vaddr;
            uint64_t existing_end = existing_start + g_vm_regions[i].length;

            if (req->vaddr < existing_end && existing_start < req->vaddr + req->size) {
                resp->status = BHARAT_IPC_STATUS_ERR_INVALID;
                return BHARAT_IPC_STATUS_ERR_INVALID;
            }
        }
    }

    // Reserve a region slot
    bh_vm_region_v1_t *region = NULL;
    for (int i = 0; i < MAX_REGIONS; i++) {
        if (!g_vm_regions[i].in_use) {
            region = &g_vm_regions[i];
            break;
        }
    }

    if (!region) {
        resp->status = BHARAT_IPC_STATUS_ERR_LENGTH;
        return BHARAT_IPC_STATUS_ERR_LENGTH;
    }

    // Call canonical authority MAP
    bh_vm_kernel_space_ref_t k_space = { .space_id = space->kernel_space_id };
    int k_res = g_authority_ops.map(g_authority_ops.ctx, k_space, req);
    if (k_res != 0) {
        resp->status = BHARAT_IPC_STATUS_ERR_INTERNAL;
        return BHARAT_IPC_STATUS_ERR_INTERNAL;
    }

    region->in_use = true;
    region->parent_space = req->vm_space_handle;
    region->kernel_space_id = space->kernel_space_id;
    region->vaddr = req->vaddr;
    region->length = req->size;
    region->protection = req->protection;
    region->memory_type = req->memory_type;
    region->state = BH_VM_REGION_STATE_MAPPED_V1;

    bh_handle_t h_region = 0;
    int h_status = bh_user_handle_alloc(&g_vm_region_handle_table, region, BHARAT_CAP_OBJ_VM_SPACE, BHARAT_CAP_RIGHT_WRITE | BHARAT_CAP_RIGHT_READ, &h_region);
    if (h_status != BH_HANDLE_TABLE_SUCCESS) {
        g_authority_ops.unmap(g_authority_ops.ctx, k_space, req->vaddr, req->size);
        region->in_use = false;
        resp->status = BHARAT_IPC_STATUS_ERR_LENGTH;
        return BHARAT_IPC_STATUS_ERR_LENGTH;
    }

    region->public_handle = h_region;

    resp->status = BHARAT_IPC_STATUS_OK;
    resp->region_handle = h_region;
    return BHARAT_IPC_STATUS_OK;
}

int32_t bh_vm_handle_unmap_v1(const bh_vm_unmap_request_v1_t *req, bh_vm_unmap_response_v1_t *resp) {
    if (!req || !resp) {
        return BHARAT_IPC_STATUS_ERR_INVALID;
    }

    local_memset(resp, 0, sizeof(bh_vm_unmap_response_v1_t));
    resp->abi_version = BH_VM_INTERFACE_VERSION_V1;
    resp->struct_size = sizeof(bh_vm_unmap_response_v1_t);

    bh_vm_space_v1_t *space = NULL;
    int h_res = bh_user_handle_lookup(&g_vm_space_handle_table, req->vm_space_handle, BHARAT_CAP_OBJ_VM_SPACE, (void **)&space, NULL);
    if (h_res != BH_HANDLE_TABLE_SUCCESS || !space) {
        resp->status = BHARAT_IPC_STATUS_ERR_NOT_FOUND;
        return BHARAT_IPC_STATUS_ERR_NOT_FOUND;
    }

    bh_vm_region_v1_t *region = NULL;
    h_res = bh_user_handle_lookup(&g_vm_region_handle_table, req->region_handle, BHARAT_CAP_OBJ_VM_SPACE, (void **)&region, NULL);
    if (h_res != BH_HANDLE_TABLE_SUCCESS || !region) {
        resp->status = BHARAT_IPC_STATUS_ERR_NOT_FOUND;
        return BHARAT_IPC_STATUS_ERR_NOT_FOUND;
    }

    if (region->parent_space != req->vm_space_handle) {
        resp->status = BHARAT_IPC_STATUS_ERR_INVALID;
        return BHARAT_IPC_STATUS_ERR_INVALID;
    }

    // Call canonical authority UNMAP
    bh_vm_kernel_space_ref_t k_space = { .space_id = space->kernel_space_id };
    int k_res = g_authority_ops.unmap(g_authority_ops.ctx, k_space, region->vaddr, region->length);
    if (k_res != 0) {
        resp->status = BHARAT_IPC_STATUS_ERR_INTERNAL;
        return BHARAT_IPC_STATUS_ERR_INTERNAL;
    }

    region->state = BH_VM_REGION_STATE_REVOKED_V1;

    bh_user_handle_revoke(&g_vm_region_handle_table, req->region_handle);
    local_memset(region, 0, sizeof(bh_vm_region_v1_t));

    resp->status = BHARAT_IPC_STATUS_OK;
    return BHARAT_IPC_STATUS_OK;
}

int32_t bh_vm_handle_protect_v1(const bh_vm_protect_request_v1_t *req, bh_vm_protect_response_v1_t *resp) {
    if (!req || !resp) {
        return BHARAT_IPC_STATUS_ERR_INVALID;
    }

    local_memset(resp, 0, sizeof(bh_vm_protect_response_v1_t));
    resp->abi_version = BH_VM_INTERFACE_VERSION_V1;
    resp->struct_size = sizeof(bh_vm_protect_response_v1_t);

    // W^X check
    if ((req->new_protection & 2) && (req->new_protection & 4)) {
        resp->status = BHARAT_IPC_STATUS_ERR_INVALID;
        return BHARAT_IPC_STATUS_ERR_INVALID;
    }

    bh_vm_space_v1_t *space = NULL;
    int h_res = bh_user_handle_lookup(&g_vm_space_handle_table, req->vm_space_handle, BHARAT_CAP_OBJ_VM_SPACE, (void **)&space, NULL);
    if (h_res != BH_HANDLE_TABLE_SUCCESS || !space) {
        resp->status = BHARAT_IPC_STATUS_ERR_NOT_FOUND;
        return BHARAT_IPC_STATUS_ERR_NOT_FOUND;
    }

    bh_vm_region_v1_t *region = NULL;
    h_res = bh_user_handle_lookup(&g_vm_region_handle_table, req->region_handle, BHARAT_CAP_OBJ_VM_SPACE, (void **)&region, NULL);
    if (h_res != BH_HANDLE_TABLE_SUCCESS || !region) {
        resp->status = BHARAT_IPC_STATUS_ERR_NOT_FOUND;
        return BHARAT_IPC_STATUS_ERR_NOT_FOUND;
    }

    if (region->parent_space != req->vm_space_handle) {
        resp->status = BHARAT_IPC_STATUS_ERR_INVALID;
        return BHARAT_IPC_STATUS_ERR_INVALID;
    }

    // Call canonical authority protect
    bh_vm_kernel_space_ref_t k_space = { .space_id = space->kernel_space_id };
    int k_res = g_authority_ops.protect(g_authority_ops.ctx, k_space, region->vaddr, region->length, req->new_protection, region->memory_type);
    if (k_res != 0) {
        resp->status = BHARAT_IPC_STATUS_ERR_INTERNAL;
        return BHARAT_IPC_STATUS_ERR_INTERNAL;
    }

    // Updatecached protection only after success
    region->protection = req->new_protection;

    resp->status = BHARAT_IPC_STATUS_OK;
    return BHARAT_IPC_STATUS_OK;
}

int32_t bh_vm_handle_query_v1(const bh_vm_query_request_v1_t *req, bh_vm_query_response_v1_t *resp) {
    if (!req || !resp) {
        return BHARAT_IPC_STATUS_ERR_INVALID;
    }

    local_memset(resp, 0, sizeof(bh_vm_query_response_v1_t));
    resp->abi_version = BH_VM_INTERFACE_VERSION_V1;
    resp->struct_size = sizeof(bh_vm_query_response_v1_t);

    bh_vm_space_v1_t *space = NULL;
    int h_res = bh_user_handle_lookup(&g_vm_space_handle_table, req->vm_space_handle, BHARAT_CAP_OBJ_VM_SPACE, (void **)&space, NULL);
    if (h_res != BH_HANDLE_TABLE_SUCCESS || !space) {
        resp->status = BHARAT_IPC_STATUS_ERR_NOT_FOUND;
        return BHARAT_IPC_STATUS_ERR_NOT_FOUND;
    }

    bh_vm_region_v1_t *region = NULL;
    h_res = bh_user_handle_lookup(&g_vm_region_handle_table, req->region_handle, BHARAT_CAP_OBJ_VM_SPACE, (void **)&region, NULL);
    if (h_res != BH_HANDLE_TABLE_SUCCESS || !region) {
        resp->status = BHARAT_IPC_STATUS_ERR_NOT_FOUND;
        return BHARAT_IPC_STATUS_ERR_NOT_FOUND;
    }

    if (region->parent_space != req->vm_space_handle) {
        resp->status = BHARAT_IPC_STATUS_ERR_INVALID;
        return BHARAT_IPC_STATUS_ERR_INVALID;
    }

    resp->status = BHARAT_IPC_STATUS_OK;
    resp->state = region->state;
    resp->vaddr = region->vaddr;
    resp->size = region->length;
    resp->protection = region->protection;
    resp->memory_type = region->memory_type;

    return BHARAT_IPC_STATUS_OK;
}

// -----------------------------------------------------------------------------
// IPC Dispatch Loop for versioned UAPI
// -----------------------------------------------------------------------------

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

        // Route between legacy v0 and modern v1 interface versions
        if (req_header.interface_version == BH_VM_INTERFACE_VERSION_V1) {
            switch (req_header.opcode) {
                case BH_VM_OP_CREATE_SPACE_V1: {
                    if (req_header.payload_size >= sizeof(bh_vm_create_space_request_v1_t)) {
                        const bh_vm_create_space_request_v1_t *req = (const bh_vm_create_space_request_v1_t *)payload_buf;
                        bh_vm_create_space_response_v1_t *resp = (bh_vm_create_space_response_v1_t *)resp_payload_buf;
                        dispatch_status = bh_vm_handle_create_space_v1(req, resp);
                        resp_size = sizeof(bh_vm_create_space_response_v1_t);
                    } else {
                        dispatch_status = BHARAT_IPC_STATUS_ERR_LENGTH;
                    }
                    break;
                }
                case BH_VM_OP_DESTROY_SPACE_V1: {
                    if (req_header.payload_size >= sizeof(bh_vm_destroy_space_request_v1_t)) {
                        const bh_vm_destroy_space_request_v1_t *req = (const bh_vm_destroy_space_request_v1_t *)payload_buf;
                        bh_vm_destroy_space_response_v1_t *resp = (bh_vm_destroy_space_response_v1_t *)resp_payload_buf;
                        dispatch_status = bh_vm_handle_destroy_space_v1(req, resp);
                        resp_size = sizeof(bh_vm_destroy_space_response_v1_t);
                    } else {
                        dispatch_status = BHARAT_IPC_STATUS_ERR_LENGTH;
                    }
                    break;
                }
                case BH_VM_OP_MAP_V1: {
                    if (req_header.payload_size >= sizeof(bh_vm_map_request_v1_t)) {
                        const bh_vm_map_request_v1_t *req = (const bh_vm_map_request_v1_t *)payload_buf;
                        bh_vm_map_response_v1_t *resp = (bh_vm_map_response_v1_t *)resp_payload_buf;
                        dispatch_status = bh_vm_handle_map_v1(req, resp);
                        resp_size = sizeof(bh_vm_map_response_v1_t);
                    } else {
                        dispatch_status = BHARAT_IPC_STATUS_ERR_LENGTH;
                    }
                    break;
                }
                case BH_VM_OP_UNMAP_V1: {
                    if (req_header.payload_size >= sizeof(bh_vm_unmap_request_v1_t)) {
                        const bh_vm_unmap_request_v1_t *req = (const bh_vm_unmap_request_v1_t *)payload_buf;
                        bh_vm_unmap_response_v1_t *resp = (bh_vm_unmap_response_v1_t *)resp_payload_buf;
                        dispatch_status = bh_vm_handle_unmap_v1(req, resp);
                        resp_size = sizeof(bh_vm_unmap_response_v1_t);
                    } else {
                        dispatch_status = BHARAT_IPC_STATUS_ERR_LENGTH;
                    }
                    break;
                }
                case BH_VM_OP_PROTECT_V1: {
                    if (req_header.payload_size >= sizeof(bh_vm_protect_request_v1_t)) {
                        const bh_vm_protect_request_v1_t *req = (const bh_vm_protect_request_v1_t *)payload_buf;
                        bh_vm_protect_response_v1_t *resp = (bh_vm_protect_response_v1_t *)resp_payload_buf;
                        dispatch_status = bh_vm_handle_protect_v1(req, resp);
                        resp_size = sizeof(bh_vm_protect_response_v1_t);
                    } else {
                        dispatch_status = BHARAT_IPC_STATUS_ERR_LENGTH;
                    }
                    break;
                }
                case BH_VM_OP_QUERY_V1: {
                    if (req_header.payload_size >= sizeof(bh_vm_query_request_v1_t)) {
                        const bh_vm_query_request_v1_t *req = (const bh_vm_query_request_v1_t *)payload_buf;
                        bh_vm_query_response_v1_t *resp = (bh_vm_query_response_v1_t *)resp_payload_buf;
                        dispatch_status = bh_vm_handle_query_v1(req, resp);
                        resp_size = sizeof(bh_vm_query_response_v1_t);
                    } else {
                        dispatch_status = BHARAT_IPC_STATUS_ERR_LENGTH;
                    }
                    break;
                }
                default:
                    break;
            }
        } else {
            // Legacy v0 interface
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
        }

        resp_header.payload_size = resp_size;
        resp_header.flags = dispatch_status;

        if (req_header.reply_endpoint != 0) {
            bharat_ipc_endpoint_t rep_ep = req_header.reply_endpoint;
            bharat_ipc_send(rep_ep, &resp_header, resp_payload_buf);
        }
    }
}
