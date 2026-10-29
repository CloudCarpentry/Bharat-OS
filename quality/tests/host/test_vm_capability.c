#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "capability.h"
#include "cap_policy.h"
#include "mm/vm_cap_dispatch.h"
#include "syscall/usercopy.h"
#include <bharat/uapi/system/vmm.h>

static bh_process_t process;
static capability_table_t table;
static capability_entry_t authority;
static kstatus_t validation_status;
static bool copy_fault;
bh_process_t *sched_current_process(void) { return &process; }
uint32_t hal_cpu_get_id(void) { return 0; }
kstatus_t cap_validate_ex(capability_table_t *cspace, const cap_validation_request_t *req, capability_entry_t *out) {
    assert(cspace == &table && req->requester_pid == process.process_id);
    if (validation_status) return validation_status;
    if (req->expected_object_type != authority.type) return K_ERR_CAP_WRONG_TYPE;
    if ((authority.rights & req->required_rights) != req->required_rights) return K_ERR_CAP_DENIED;
    *out = authority; return K_OK;
}
bh_status_t bh_copy_from_user(void *dst, const void *src, size_t len) {
    if (copy_fault || !src) return BH_ERR_FAULT;
    memcpy(dst, src, len); return BH_OK;
}
kstatus_t bh_status_to_kstatus(bh_status_t status) { return status == BH_OK ? K_OK : K_ERR_FAULT; }
static int invoke(uint32_t opcode, bh_vm_request_t *req) {
    return vm_space_cap_invoke(0x10001, opcode, (uintptr_t)req, 0);
}
int main(void) {
    process.process_id=7; process.security_sandbox_ctx=&table;
    table.owner_pid=7; table.owner_core=0;
    authority.type=CAP_TYPE_PROCESS; authority.object_ref=(uintptr_t)&process;
    authority.rights=CAP_RIGHT_RESOURCE_ALLOC;
    bh_vm_request_t req={.version=BH_VM_ABI_VERSION,.struct_size=sizeof(req)};
    /* These are denial/contract tests, not evidence of working VM lifecycle. */
    assert(invoke(BH_VM_OP_CREATE,&req)==K_ERR_UNSUPPORTED);
    authority.rights=0; assert(invoke(BH_VM_OP_CREATE,&req)==K_ERR_CAP_DENIED);
    authority.rights=CAP_RIGHT_RESOURCE_ALLOC;
    authority.object_ref=0; assert(invoke(BH_VM_OP_CREATE,&req)==K_ERR_CAP_OWNERSHIP);
    authority.type=CAP_TYPE_VM_SPACE;
    authority.rights=CAP_RIGHT_MEMORY_MAP|CAP_RIGHT_MEMORY_UNMAP|CAP_RIGHT_VMM_MANAGE|CAP_RIGHT_QUERY;
    assert(cap_transfer_rights_valid(authority.type,authority.rights));
    assert(!cap_transfer_rights_valid(authority.type,CAP_RIGHT_DELEGATE));
    assert(!cap_transfer_rights_valid(authority.type,CAP_RIGHT_ENDPOINT_SEND));
    req.protection=BH_VM_PROT_WRITE|BH_VM_PROT_EXEC;
    assert(invoke(BH_VM_OP_MAP,&req)==K_ERR_VM_PROT);
    assert(invoke(BH_VM_OP_PROTECT,&req)==K_ERR_VM_PROT);
    req.protection=BH_VM_PROT_READ; req.address=UINT64_MAX; req.length=2;
    assert(invoke(BH_VM_OP_MAP,&req)==K_ERR_OVERFLOW);
    req.address=1; req.length=4096;
    assert(invoke(BH_VM_OP_MAP,&req)==K_ERR_ALIGNMENT);
    req.address=4096;
    for (uint32_t op=BH_VM_OP_DESTROY;op<=BH_VM_OP_QUERY;++op) assert(invoke(op,&req)==K_ERR_UNSUPPORTED);
    req.profile=0; assert(invoke(BH_VM_OP_MAP,&req)==K_ERR_UNSUPPORTED);
    validation_status=K_ERR_CAP_STALE; assert(invoke(BH_VM_OP_MAP,&req)==K_ERR_CAP_STALE);
    validation_status=K_ERR_CAP_REVOKED; assert(invoke(BH_VM_OP_MAP,&req)==K_ERR_CAP_REVOKED);
    validation_status=0; authority.type=CAP_TYPE_ENDPOINT;
    assert(invoke(BH_VM_OP_MAP,&req)==K_ERR_CAP_WRONG_TYPE);
    authority.type=CAP_TYPE_VM_SPACE;
    table.owner_pid=8; assert(invoke(BH_VM_OP_MAP,&req)==K_ERR_CAP_OWNERSHIP); table.owner_pid=7;
    copy_fault=true; assert(invoke(BH_VM_OP_MAP,&req)==K_ERR_FAULT); copy_fault=false;
    req.struct_size=1; assert(invoke(BH_VM_OP_MAP,&req)==K_ERR_INVALID_ARG);
    puts("VM capability fail-closed contract passed; lifecycle activation remains blocked");
}
