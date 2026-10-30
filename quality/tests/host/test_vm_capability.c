#include "cap_policy.h"
#include "capability.h"
#include "sched/sched.h"
#include "mm/aspace.h"
#include "mm/prot_domain.h"
#include "mm/vm_cap_dispatch.h"
#include "mm/vm_mapping.h"
#include "syscall/usercopy.h"
#include <assert.h>
#include <bharat/uapi/system/vmm.h>
#include <stdio.h>
#include <string.h>

static bh_process_t process;
static capability_table_t table;
static capability_entry_t authority;
static capability_entry_t backing;
static kstatus_t validation_status;
static bool copy_fault;
static uint32_t active_pin_count;

/* Mock address space and protection domain for positive/negative VM lifecycle
 * tests */
static address_space_t mock_aspace;
static prot_domain_t mock_pd;
static vm_region_t mock_region;
static bool mock_region_in_use;
static uintptr_t mock_pd_vaddr;
static uintptr_t mock_pd_paddr;
static size_t mock_pd_size;
static uint32_t mock_pd_flags;
static bool mock_pd_mapped;
static bool mock_pd_fail_map;

bh_process_t *sched_current_process(void) { return &process; }
uint32_t hal_cpu_get_id(void) { return 0; }

kstatus_t cap_validate_ex(capability_table_t *cspace,
                          const cap_validation_request_t *req,
                          capability_entry_t *out) {
  assert(cspace == &table && req->requester_pid == process.process_id);
  if (validation_status)
    return validation_status;
  capability_entry_t *target =
      (req->cap_id == 0x20002U) ? &backing : &authority;
  if (req->expected_object_type != CAP_TYPE_NONE &&
      req->expected_object_type != target->type)
    return K_ERR_CAP_WRONG_TYPE;
  if ((target->rights & req->required_rights) != req->required_rights)
    return K_ERR_CAP_DENIED;
  *out = *target;
  return K_OK;
}

kstatus_t cap_validate_and_pin(capability_table_t *cspace,
                               const cap_validation_request_t *req,
                               capability_entry_t *out_entry,
                               uint32_t *out_pinned_slot) {
  kstatus_t st = cap_validate_ex(cspace, req, out_entry);
  if (st == K_OK) {
    active_pin_count++;
    *out_pinned_slot = (req->cap_id == 0x20002U) ? 2U : 1U;
  }
  return st;
}

void cap_unpin(capability_table_t *cspace, uint32_t pinned_slot) {
  assert(cspace == &table);
  assert(pinned_slot == 1U || pinned_slot == 2U);
  assert(active_pin_count > 0U);
  active_pin_count--;
}

int aspace_region_reserve(address_space_t *aspace, uintptr_t base,
                          size_t length, uint32_t prot, uint32_t map_flags,
                          vm_inherit_t inherit, vm_region_t **out_region) {
  (void)map_flags;
  (void)inherit;
  assert(aspace == &mock_aspace);
  if (mock_region_in_use && !(base + length <= mock_region.base ||
                              base >= mock_region.base + mock_region.length)) {
    return K_ERR_VM_ALREADY_MAPPED;
  }
  mock_region.base = base;
  mock_region.length = length;
  mock_region.prot = prot;
  mock_region_in_use = true;
  if (out_region)
    *out_region = &mock_region;
  return K_OK;
}

int aspace_region_detach(address_space_t *aspace, uintptr_t base) {
  assert(aspace == &mock_aspace);
  if (!mock_region_in_use || mock_region.base != base)
    return K_ERR_NOT_FOUND;
  mock_region_in_use = false;
  return K_OK;
}

vm_region_t *aspace_lookup_region(address_space_t *aspace, uintptr_t va) {
  assert(aspace == &mock_aspace);
  if (mock_region_in_use && va >= mock_region.base &&
      va < mock_region.base + mock_region.length) {
    return &mock_region;
  }
  return NULL;
}

int prot_domain_map_region(prot_domain_t *domain, uintptr_t vaddr,
                           uintptr_t paddr, size_t size, uint32_t flags) {
  assert(domain == &mock_pd);
  if (mock_pd_fail_map)
    return K_ERR_NO_MEMORY;
  mock_pd_vaddr = vaddr;
  mock_pd_paddr = paddr;
  mock_pd_size = size;
  mock_pd_flags = flags;
  mock_pd_mapped = true;
  return K_OK;
}

int prot_domain_unmap_region(prot_domain_t *domain, uintptr_t vaddr,
                             size_t size) {
  assert(domain == &mock_pd);
  if (!mock_pd_mapped || mock_pd_vaddr != vaddr || mock_pd_size != size) {
    return K_ERR_VM_UNMAPPED;
  }
  mock_pd_mapped = false;
  return K_OK;
}

int prot_domain_protect_region(prot_domain_t *domain, uintptr_t vaddr,
                               size_t size, uint32_t flags) {
  assert(domain == &mock_pd);
  if (!mock_pd_mapped || mock_pd_vaddr != vaddr || mock_pd_size != size) {
    return K_ERR_VM_UNMAPPED;
  }
  mock_pd_flags = flags;
  return K_OK;
}

int prot_domain_query_region(prot_domain_t *domain, uintptr_t vaddr,
                             uintptr_t *paddr, uint32_t *flags) {
  assert(domain == &mock_pd);
  if (!mock_pd_mapped || vaddr < mock_pd_vaddr ||
      vaddr >= mock_pd_vaddr + mock_pd_size) {
    return K_ERR_VM_UNMAPPED;
  }
  if (paddr)
    *paddr = mock_pd_paddr + (vaddr - mock_pd_vaddr);
  if (flags)
    *flags = mock_pd_flags;
  return K_OK;
}

bh_status_t bh_copy_from_user(void *dst, const void *src, size_t len) {
  if (copy_fault || !src)
    return BH_ERR_FAULT;
  memcpy(dst, src, len);
  return BH_OK;
}
kstatus_t bh_status_to_kstatus(bh_status_t status) {
  return status == BH_OK ? K_OK : K_ERR_FAULT;
}
static int invoke(uint32_t opcode, bh_vm_request_t *req) {
  return vm_space_cap_invoke(0x10001, opcode, (uintptr_t)req, 0);
}
int main(void) {
  process.process_id = 7;
  process.security_sandbox_ctx = &table;
  table.owner_pid = 7;
  table.owner_core = 0;
  authority.type = CAP_TYPE_PROCESS;
  authority.object_ref = (uintptr_t)&process;
  authority.rights = CAP_RIGHT_RESOURCE_ALLOC;
  bh_vm_request_t req = {.version = BH_VM_ABI_VERSION,
                         .struct_size = sizeof(req)};
  /* 1. Fail-closed denial and contract tests with unbacked object_ref */
  assert(invoke(BH_VM_OP_CREATE, &req) == K_ERR_UNSUPPORTED);
  assert(active_pin_count == 0U);
  authority.rights = 0;
  assert(invoke(BH_VM_OP_CREATE, &req) == K_ERR_CAP_DENIED);
  authority.rights = CAP_RIGHT_RESOURCE_ALLOC;
  authority.object_ref = 0;
  assert(invoke(BH_VM_OP_CREATE, &req) == K_ERR_CAP_OWNERSHIP);
  authority.type = CAP_TYPE_VM_SPACE;
  authority.rights = CAP_RIGHT_MEMORY_MAP | CAP_RIGHT_MEMORY_UNMAP |
                     CAP_RIGHT_VMM_MANAGE | CAP_RIGHT_QUERY;
  assert(cap_transfer_rights_valid(authority.type, authority.rights));
  assert(!cap_transfer_rights_valid(authority.type, CAP_RIGHT_DELEGATE));
  assert(!cap_transfer_rights_valid(authority.type, CAP_RIGHT_ENDPOINT_SEND));
  req.protection = BH_VM_PROT_WRITE | BH_VM_PROT_EXEC;
  assert(invoke(BH_VM_OP_MAP, &req) == K_ERR_VM_PROT);
  assert(invoke(BH_VM_OP_PROTECT, &req) == K_ERR_VM_PROT);
  req.protection = BH_VM_PROT_READ;
  req.address = UINT64_MAX;
  req.length = 2;
  assert(invoke(BH_VM_OP_MAP, &req) == K_ERR_OVERFLOW);
  req.address = 1;
  req.length = 4096;
  assert(invoke(BH_VM_OP_MAP, &req) == K_ERR_ALIGNMENT);
  req.address = 4096;
  for (uint32_t op = BH_VM_OP_DESTROY; op <= BH_VM_OP_QUERY; ++op)
    assert(invoke(op, &req) == K_ERR_UNSUPPORTED);
  req.profile = 0;
  assert(invoke(BH_VM_OP_MAP, &req) == K_ERR_UNSUPPORTED);
  validation_status = K_ERR_CAP_STALE;
  assert(invoke(BH_VM_OP_MAP, &req) == K_ERR_CAP_STALE);
  validation_status = K_ERR_CAP_REVOKED;
  assert(invoke(BH_VM_OP_MAP, &req) == K_ERR_CAP_REVOKED);
  validation_status = 0;
  authority.type = CAP_TYPE_ENDPOINT;
  assert(invoke(BH_VM_OP_MAP, &req) == K_ERR_CAP_WRONG_TYPE);
  authority.type = CAP_TYPE_VM_SPACE;
  table.owner_pid = 8;
  assert(invoke(BH_VM_OP_MAP, &req) == K_ERR_CAP_OWNERSHIP);
  table.owner_pid = 7;
  copy_fault = true;
  assert(invoke(BH_VM_OP_MAP, &req) == K_ERR_FAULT);
  copy_fault = false;
  req.struct_size = 1;
  assert(invoke(BH_VM_OP_MAP, &req) == K_ERR_INVALID_ARG);
  req.struct_size = sizeof(req);
  assert(active_pin_count == 0U);

  /* 2. Positive & negative VM capability lifecycle tests (MAP / PROTECT / QUERY
   * / UNMAP) */
  mock_aspace.owner_core_id = 0U;
  mock_aspace.state = ASPACE_STATE_ACTIVE;
  mock_aspace.prot_domain = &mock_pd;
  mock_aspace.user_base = 0x1000ULL;
  mock_aspace.user_limit = 0x7FFFFFFFULL;
  authority.object_ref = (uintptr_t)&mock_aspace;

  backing.type = CAP_TYPE_MEMORY;
  backing.rights = CAP_RIGHT_MEMORY_MAP | CAP_RIGHT_MEMORY_UNMAP;
  backing.object_ref = 0x80000000ULL;
  backing.scope_id = 8192U; /* 2 pages backing extent */

  req.address = 0x10000ULL;
  req.length = 4096ULL;
  req.backing_offset = 0ULL;
  req.protection = BH_VM_PROT_READ;
  req.backing_cap = 0x20002U;

  /* Out-of-bounds backing extent must fail closed with K_ERR_OVERFLOW */
  req.backing_offset = 8192ULL;
  assert(invoke(BH_VM_OP_MAP, &req) == K_ERR_OVERFLOW);
  assert(!mock_region_in_use && !mock_pd_mapped);
  assert(active_pin_count == 0U);

  /* Wrong backing capability type must fail closed */
  req.backing_offset = 0ULL;
  backing.type = CAP_TYPE_ENDPOINT;
  assert(invoke(BH_VM_OP_MAP, &req) == K_ERR_CAP_WRONG_TYPE);
  backing.type = CAP_TYPE_MEMORY;
  assert(active_pin_count == 0U);

  /* Protection domain failure rolls back region reservation cleanly */
  mock_pd_fail_map = true;
  assert(invoke(BH_VM_OP_MAP, &req) == K_ERR_NO_MEMORY);
  assert(!mock_region_in_use && !mock_pd_mapped);
  assert(active_pin_count == 0U);
  mock_pd_fail_map = false;

  /* Valid MAP succeeds and pins/unpins capabilities */
  assert(invoke(BH_VM_OP_MAP, &req) == K_OK);
  assert(mock_region_in_use && mock_pd_mapped);
  assert(mock_pd_vaddr == 0x10000ULL && mock_pd_paddr == 0x80000000ULL);
  assert(active_pin_count == 0U);

  /* Overlapping MAP fails closed */
  assert(invoke(BH_VM_OP_MAP, &req) == K_ERR_VM_ALREADY_MAPPED);
  assert(active_pin_count == 0U);

  /* QUERY on mapped region succeeds */
  assert(invoke(BH_VM_OP_QUERY, &req) == K_OK);
  assert(active_pin_count == 0U);

  /* PROTECT updates region and protection domain */
  req.protection = BH_VM_PROT_READ | BH_VM_PROT_WRITE;
  assert(invoke(BH_VM_OP_PROTECT, &req) == K_OK);
  assert((mock_pd_flags & (VM_PROT_READ | VM_PROT_WRITE)) ==
         (VM_PROT_READ | VM_PROT_WRITE));
  assert(active_pin_count == 0U);

  /* UNMAP removes mapping and detaches region */
  assert(invoke(BH_VM_OP_UNMAP, &req) == K_OK);
  assert(!mock_region_in_use && !mock_pd_mapped);
  assert(active_pin_count == 0U);

  /* QUERY after UNMAP returns K_ERR_VM_UNMAPPED */
  assert(invoke(BH_VM_OP_QUERY, &req) == K_ERR_VM_UNMAPPED);
  assert(active_pin_count == 0U);

  puts("VM capability fail-closed and lifecycle contract passed");
  return 0;
}
