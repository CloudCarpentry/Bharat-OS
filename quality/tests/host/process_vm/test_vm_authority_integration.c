#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../../../../core/services/vm_manager/vm_manager.h"
#include <bharat/uapi/ipc/status.h>

static int g_mapped_count = 0;
static int g_unmapped_count = 0;
static int g_protected_count = 0;

static int32_t auth_space_create(void *ctx, const bh_vm_create_space_request_v1_t *req, bh_vm_kernel_space_ref_t *out_ref) {
    (void)ctx; (void)req;
    out_ref->space_id = 9988;
    return 0;
}

static int32_t auth_space_destroy(void *ctx, bh_vm_kernel_space_ref_t ref) {
    (void)ctx; (void)ref;
    return 0;
}

static int32_t auth_map(void *ctx, bh_vm_kernel_space_ref_t ref, const bh_vm_map_request_v1_t *req) {
    (void)ctx; (void)ref; (void)req;
    g_mapped_count++;
    return 0;
}

static int32_t auth_unmap(void *ctx, bh_vm_kernel_space_ref_t ref, uint64_t vaddr, uint64_t length) {
    (void)ctx; (void)ref; (void)vaddr; (void)length;
    g_unmapped_count++;
    return 0;
}

static int32_t auth_protect(void *ctx, bh_vm_kernel_space_ref_t ref, uint64_t vaddr, uint64_t length, uint64_t protection, uint64_t memory_type) {
    (void)ctx; (void)ref; (void)vaddr; (void)length; (void)protection; (void)memory_type;
    g_protected_count++;
    return 0;
}

static int32_t auth_query(void *ctx, bh_vm_kernel_space_ref_t ref, uint64_t vaddr, bh_vm_kernel_query_result_t *out_res) {
    (void)ctx; (void)ref; (void)vaddr;
    return 0;
}

void test_vm_authority_integration_v1(void) {
    vm_manager_init();

    bh_vm_authority_ops_t ops = {
        .ctx = NULL,
        .space_create = auth_space_create,
        .space_destroy = auth_space_destroy,
        .map = auth_map,
        .unmap = auth_unmap,
        .protect = auth_protect,
        .query = auth_query
    };
    bh_vm_set_authority_ops(&ops);

    g_mapped_count = 0;
    g_unmapped_count = 0;
    g_protected_count = 0;

    // Create space (profile MMU, soft RT)
    bh_vm_create_space_request_v1_t c_req;
    memset(&c_req, 0, sizeof(c_req));
    c_req.abi_version = BH_VM_INTERFACE_VERSION_V1;
    c_req.struct_size = sizeof(c_req);
    c_req.memory_profile = 2; // MMU profile
    c_req.timing_class = 2; // Soft RT

    bh_vm_create_space_response_v1_t c_resp;
    int status = bh_vm_handle_create_space_v1(&c_req, &c_resp);
    assert(status == BHARAT_IPC_STATUS_OK);
    assert(c_resp.vm_space_handle != 0);

    bh_vm_space_handle_t space_handle = c_resp.vm_space_handle;
    assert(bh_vm_get_active_spaces_count() == 1);

    // Map region 1 (Vaddr: 0x10000, Size: 0x2000, protection: READ|WRITE=3)
    bh_vm_map_request_v1_t m_req;
    memset(&m_req, 0, sizeof(m_req));
    m_req.abi_version = BH_VM_INTERFACE_VERSION_V1;
    m_req.struct_size = sizeof(m_req);
    m_req.vm_space_handle = space_handle;
    m_req.vaddr = 0x10000;
    m_req.size = 0x2000;
    m_req.protection = 3;

    bh_vm_map_response_v1_t m_resp;
    status = bh_vm_handle_map_v1(&m_req, &m_resp);
    assert(status == BHARAT_IPC_STATUS_OK);
    assert(m_resp.region_handle != 0);
    assert(g_mapped_count == 1);
    assert(bh_vm_get_active_regions_count() == 1);

    bh_vm_region_handle_t region_handle = m_resp.region_handle;

    // Reject overlapping map
    status = bh_vm_handle_map_v1(&m_req, &m_resp);
    assert(status == BHARAT_IPC_STATUS_ERR_INVALID);

    // Reject non-aligned map
    m_req.vaddr = 0x10050; // offset misalignment
    status = bh_vm_handle_map_v1(&m_req, &m_resp);
    assert(status == BHARAT_IPC_STATUS_ERR_INVALID);

    // Reject zero size map
    m_req.vaddr = 0x20000;
    m_req.size = 0;
    status = bh_vm_handle_map_v1(&m_req, &m_resp);
    assert(status == BHARAT_IPC_STATUS_ERR_INVALID);

    // Reject W^X mapping (2 is PF_W, 4 is PF_X -> 6 is both)
    m_req.size = 0x1000;
    m_req.protection = 6;
    status = bh_vm_handle_map_v1(&m_req, &m_resp);
    assert(status == BHARAT_IPC_STATUS_ERR_INVALID);

    // Protect region 1 (READ-only=1)
    bh_vm_protect_request_v1_t p_req;
    memset(&p_req, 0, sizeof(p_req));
    p_req.abi_version = BH_VM_INTERFACE_VERSION_V1;
    p_req.struct_size = sizeof(p_req);
    p_req.vm_space_handle = space_handle;
    p_req.region_handle = region_handle;
    p_req.new_protection = 1;

    bh_vm_protect_response_v1_t p_resp;
    status = bh_vm_handle_protect_v1(&p_req, &p_resp);
    assert(status == BHARAT_IPC_STATUS_OK);
    assert(g_protected_count == 1);

    // Reject W^X protection modification
    p_req.new_protection = 6;
    status = bh_vm_handle_protect_v1(&p_req, &p_resp);
    assert(status == BHARAT_IPC_STATUS_ERR_INVALID);

    // Unmap region 1
    bh_vm_unmap_request_v1_t u_req;
    memset(&u_req, 0, sizeof(u_req));
    u_req.abi_version = BH_VM_INTERFACE_VERSION_V1;
    u_req.struct_size = sizeof(u_req);
    u_req.vm_space_handle = space_handle;
    u_req.region_handle = region_handle;

    bh_vm_unmap_response_v1_t u_resp;
    status = bh_vm_handle_unmap_v1(&u_req, &u_resp);
    assert(status == BHARAT_IPC_STATUS_OK);
    assert(g_unmapped_count == 1);
    assert(bh_vm_get_active_regions_count() == 0);

    // Double-unmap fails (stale handle)
    status = bh_vm_handle_unmap_v1(&u_req, &u_resp);
    assert(status == BHARAT_IPC_STATUS_ERR_NOT_FOUND);

    // Destroy space
    bh_vm_destroy_space_request_v1_t d_req;
    memset(&d_req, 0, sizeof(d_req));
    d_req.abi_version = BH_VM_INTERFACE_VERSION_V1;
    d_req.struct_size = sizeof(d_req);
    d_req.vm_space_handle = space_handle;

    bh_vm_destroy_space_response_v1_t d_resp;
    status = bh_vm_handle_destroy_space_v1(&d_req, &d_resp);
    assert(status == BHARAT_IPC_STATUS_OK);
    assert(bh_vm_get_active_spaces_count() == 0);

    printf("test_vm_authority_integration_v1 passed!\n");
}

int main(void) {
    test_vm_authority_integration_v1();
    return 0;
}
