#include <assert.h>
#include <string.h>
#include <bharat/cap/cap_validate.h>
#include <bharat/uapi/namesvc/contract.h>
#include <bharat/uapi/ipc/contract.h>
#include <bharat/uapi/ipc/status.h>
#include <ipc_dispatch.h>
#include <registry.h>

static bool g_allow = false;
static int g_validate_calls = 0;

static bharat_cap_status_t fake_cap_validate(
    bharat_cap_handle_t handle,
    bharat_cap_object_type_t expected_object_type,
    uint64_t expected_object_id,
    uint64_t required_rights,
    const bharat_cap_scope_t *required_scope,
    bharat_cap_validation_result_t *out_result)
{
    (void)handle;
    (void)required_rights;
    g_validate_calls++;

    assert(expected_object_type == BHARAT_CAP_OBJ_SERVICE);
    assert(expected_object_id == 0);
    assert(required_scope != NULL);
    assert(required_scope->kind == BHARAT_CAP_SCOPE_SERVICE);
    assert(required_scope->scope_id == 0);

    if (out_result) {
        memset(out_result, 0, sizeof(*out_result));
        out_result->allowed = g_allow;
        out_result->status = g_allow ? BHARAT_CAP_OK : BHARAT_CAP_RIGHTS_DENIED;
        out_result->descriptor.owner_principal_id = handle;
    }
    return g_allow ? BHARAT_CAP_OK : BHARAT_CAP_RIGHTS_DENIED;
}

bharat_cap_status_t bharat_cap_validate(
    bharat_cap_handle_t handle,
    bharat_cap_object_type_t expected_object_type,
    uint64_t expected_object_id,
    uint64_t required_rights,
    const bharat_cap_scope_t *required_scope,
    bharat_cap_validation_result_t *out_result)
{
    return fake_cap_validate(handle, expected_object_type, expected_object_id, required_rights, required_scope, out_result);
}

int32_t bharat_service_dispatch_authorize(
    uint32_t target_service_id,
    uint32_t opcode,
    const void *authz_table,
    uint32_t table_entries,
    bharat_cap_handle_t capability_transfer,
    uint32_t flags) {
    if (g_allow && capability_transfer != BHARAT_CAP_INVALID_HANDLE) return 0;
    return BHARAT_IPC_STATUS_ERR_PERM;
}

static bharat_ipc_contract_header_t mk_hdr(uint32_t opcode, uint32_t payload_size, bharat_cap_handle_t cap) {
    bharat_ipc_contract_header_t hdr;
    memset(&hdr, 0, sizeof(hdr));
    hdr.opcode = opcode;
    hdr.interface_version = 1;
    hdr.payload_size = payload_size;
    hdr.capability_transfer = cap;
    return hdr;
}

int main(void) {
    namesvc_registry_init();

    namesvc_ipc_req_t req;
    namesvc_ipc_res_t res;
    bharat_ipc_contract_header_t hdr;

    // Valid lookups without a cap are permitted, so we check that it doesn't fail auth (but may return NOTFOUND)
    memset(&req, 0, sizeof(req));
    memset(&res, 0, sizeof(res));
    req.opcode = BHARAT_NAMESVC_OP_LOOKUP;
    strncpy(req.u.lookup.service_name, "com.bharat.missing", NAMESVC_MAX_NAME_LEN - 1);
    hdr = mk_hdr(BHARAT_NAMESVC_OP_LOOKUP, sizeof(struct namesvc_req_lookup), BHARAT_CAP_INVALID_HANDLE);
    g_validate_calls = 0;
    namesvc_ipc_handle_request(&hdr, &req, &res);
    assert(res.status == NAMESVC_STATUS_ERR_NOTFOUND);
    assert(g_validate_calls == 0);

    // Provide an explicitly invalid cap to operations that require one (like register)
    memset(&req, 0, sizeof(req));
    memset(&res, 0, sizeof(res));
    req.opcode = BHARAT_NAMESVC_OP_REGISTER;
    hdr = mk_hdr(BHARAT_NAMESVC_OP_REGISTER, sizeof(struct namesvc_req_register), BHARAT_CAP_INVALID_HANDLE);
    namesvc_ipc_handle_request(&hdr, &req, &res);
    assert(res.status == BHARAT_IPC_STATUS_ERR_PERM);

    // Denied validator response is surfaced as permission error.
    memset(&req, 0, sizeof(req));
    memset(&res, 0, sizeof(res));
    req.opcode = BHARAT_NAMESVC_OP_REGISTER;
    strncpy(req.u.reg.service_name, "com.bharat.test", NAMESVC_MAX_NAME_LEN - 1);
    req.u.reg.service_id = 0;
    req.u.reg.interface_version = 1;
    hdr = mk_hdr(BHARAT_NAMESVC_OP_REGISTER, sizeof(struct namesvc_req_register), 42);
    g_allow = false;
    namesvc_ipc_handle_request(&hdr, &req, &res);
    assert(res.status == BHARAT_IPC_STATUS_ERR_PERM);

    // Allowed validator path can register then lookup.
    g_allow = true;
    memset(&req, 0, sizeof(req));
    memset(&res, 0, sizeof(res));
    req.opcode = BHARAT_NAMESVC_OP_REGISTER;
    strncpy(req.u.reg.service_name, "com.bharat.test", NAMESVC_MAX_NAME_LEN - 1);
    req.u.reg.service_id = 0;
    req.u.reg.interface_version = 2;
    hdr = mk_hdr(BHARAT_NAMESVC_OP_REGISTER, sizeof(struct namesvc_req_register), 77);
    namesvc_ipc_handle_request(&hdr, &req, &res);
    assert(res.status == NAMESVC_STATUS_OK);

    memset(&req, 0, sizeof(req));
    memset(&res, 0, sizeof(res));
    req.opcode = BHARAT_NAMESVC_OP_LOOKUP;
    strncpy(req.u.lookup.service_name, "com.bharat.test", NAMESVC_MAX_NAME_LEN - 1);
    req.u.lookup.requested_version = 2;
    req.u.lookup.exact_version = true;
    hdr = mk_hdr(BHARAT_NAMESVC_OP_LOOKUP, sizeof(struct namesvc_req_lookup), 77);
    namesvc_ipc_handle_request(&hdr, &req, &res);
    assert(res.status == NAMESVC_STATUS_OK);
    assert(res.u.lookup_res.endpoint == 77);

    return 0;
}
