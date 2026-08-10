#ifndef VM_MANAGER_H
#define VM_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include <bharat/uapi/vm_manager/contract.h>
#include <bharat/ipc/ipc.h>
#include <handle_table.h>

#define MAX_REGIONS 256
#define MAX_SPACES 64

// Legacy region entry
typedef struct {
    uint32_t region_id;
    uint32_t aspace_id;
    uint64_t vaddr;
    uint64_t size;
    uint32_t flags;
    vm_region_state_t state;
    bool in_use;
} region_entry_t;

// v1 authority types
typedef struct {
    uint64_t space_id;
} bh_vm_kernel_space_ref_t;

typedef struct {
    uint32_t state;
    uint64_t vaddr;
    uint64_t size;
    uint64_t protection;
    uint64_t memory_type;
} bh_vm_kernel_query_result_t;

typedef struct {
    void *ctx;

    int32_t (*space_create)(void *ctx, const bh_vm_create_space_request_v1_t *req,
                            bh_vm_kernel_space_ref_t *out_ref);

    int32_t (*space_destroy)(void *ctx, bh_vm_kernel_space_ref_t space_ref);

    int32_t (*map)(void *ctx, bh_vm_kernel_space_ref_t space_ref,
                   const bh_vm_map_request_v1_t *req);

    int32_t (*unmap)(void *ctx, bh_vm_kernel_space_ref_t space_ref,
                     uint64_t vaddr, uint64_t length);

    int32_t (*protect)(void *ctx, bh_vm_kernel_space_ref_t space_ref,
                       uint64_t vaddr, uint64_t length,
                       uint64_t protection, uint64_t memory_type);

    int32_t (*query)(void *ctx, bh_vm_kernel_space_ref_t space_ref,
                     uint64_t vaddr, bh_vm_kernel_query_result_t *out_res);
} bh_vm_authority_ops_t;

// v1 Region Service Record
typedef struct {
    bh_vm_region_handle_t public_handle;
    bh_vm_space_handle_t parent_space;
    uint64_t kernel_space_id;
    uint64_t kernel_space_generation;
    uint64_t vaddr;
    uint64_t length;
    uint64_t protection;
    uint64_t memory_type;
    uint32_t state; // bh_vm_region_state_v1_t
    bool in_use;
} bh_vm_region_v1_t;

// v1 Space Service Record
typedef struct {
    bh_vm_space_handle_t public_handle;
    uint64_t kernel_space_id;
    uint32_t memory_profile;
    uint32_t timing_class;
    bool in_use;
} bh_vm_space_v1_t;

// Global interfaces
void vm_manager_init(void);
void vm_manager_loop(bharat_ipc_endpoint_t endpoint);
int32_t vm_manager_handle_map(const vm_req_map_t *req, vm_resp_map_t *resp);
int32_t vm_manager_handle_unmap(const vm_req_unmap_t *req, vm_resp_unmap_t *resp);
int32_t vm_manager_handle_protect(const vm_req_protect_t *req, vm_resp_protect_t *resp);
int32_t vm_manager_handle_query(const vm_req_query_t *req, vm_resp_query_t *resp);
int32_t vm_manager_handle_fault(const vm_req_fault_t *req, vm_resp_fault_t *resp);
int32_t vm_manager_authorize(uint32_t opcode, const void *req, bharat_cap_handle_t caller_cap);

// v1 global interfaces
void bh_vm_set_authority_ops(const bh_vm_authority_ops_t *ops);
int bh_vm_get_active_spaces_count(void);
int bh_vm_get_active_regions_count(void);

// Handlers for v1
int32_t bh_vm_handle_create_space_v1(const bh_vm_create_space_request_v1_t *req, bh_vm_create_space_response_v1_t *resp);
int32_t bh_vm_handle_destroy_space_v1(const bh_vm_destroy_space_request_v1_t *req, bh_vm_destroy_space_response_v1_t *resp);
int32_t bh_vm_handle_map_v1(const bh_vm_map_request_v1_t *req, bh_vm_map_response_v1_t *resp);
int32_t bh_vm_handle_unmap_v1(const bh_vm_unmap_request_v1_t *req, bh_vm_unmap_response_v1_t *resp);
int32_t bh_vm_handle_protect_v1(const bh_vm_protect_request_v1_t *req, bh_vm_protect_response_v1_t *resp);
int32_t bh_vm_handle_query_v1(const bh_vm_query_request_v1_t *req, bh_vm_query_response_v1_t *resp);

#endif // VM_MANAGER_H
