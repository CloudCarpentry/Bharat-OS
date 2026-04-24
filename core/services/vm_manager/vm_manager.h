#ifndef VM_MANAGER_H
#define VM_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include <bharat/uapi/vm_manager/contract.h>
#include <bharat/ipc/ipc.h>

#define MAX_REGIONS 256

typedef struct {
    uint32_t region_id;
    uint32_t aspace_id;
    uint64_t vaddr;
    uint64_t size;
    uint32_t flags;
    vm_region_state_t state;
    bool in_use;
} region_entry_t;

void vm_manager_init(void);
void vm_manager_loop(bharat_ipc_endpoint_t endpoint);
int32_t vm_manager_handle_map(const vm_req_map_t *req, vm_resp_map_t *resp);
int32_t vm_manager_handle_unmap(const vm_req_unmap_t *req, vm_resp_unmap_t *resp);
int32_t vm_manager_handle_protect(const vm_req_protect_t *req, vm_resp_protect_t *resp);
int32_t vm_manager_handle_query(const vm_req_query_t *req, vm_resp_query_t *resp);
int32_t vm_manager_handle_fault(const vm_req_fault_t *req, vm_resp_fault_t *resp);

#endif // VM_MANAGER_H
