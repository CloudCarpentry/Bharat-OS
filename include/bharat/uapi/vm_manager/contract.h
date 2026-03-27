#ifndef BHARAT_UAPI_VM_MANAGER_CONTRACT_H
#define BHARAT_UAPI_VM_MANAGER_CONTRACT_H

#include <stdint.h>
#include <bharat/uapi/ipc/contract.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VM_MANAGER_SERVICE_ID 0x00010002

typedef enum {
    VM_OP_MAP     = 1,
    VM_OP_UNMAP   = 2,
    VM_OP_PROTECT = 3,
    VM_OP_QUERY   = 4,
    VM_OP_FAULT   = 5
} vm_opcode_t;

typedef enum {
    VM_REGION_UNKNOWN = 0,
    VM_REGION_DECLARED,
    VM_REGION_VALIDATED,
    VM_REGION_PROGRAMMED,
    VM_REGION_ACTIVE,
    VM_REGION_REVOKED
} vm_region_state_t;

// VM_OP_MAP Request
typedef struct {
    uint32_t aspace_id;
    uint64_t vaddr;
    uint64_t size;
    uint32_t flags;
} vm_req_map_t;

// VM_OP_MAP Response
typedef struct {
    uint32_t region_id;
    int32_t status;
} vm_resp_map_t;

// VM_OP_UNMAP Request
typedef struct {
    uint32_t region_id;
} vm_req_unmap_t;

// VM_OP_UNMAP Response
typedef struct {
    int32_t status;
} vm_resp_unmap_t;

// VM_OP_PROTECT Request
typedef struct {
    uint32_t region_id;
    uint32_t new_flags;
} vm_req_protect_t;

// VM_OP_PROTECT Response
typedef struct {
    int32_t status;
} vm_resp_protect_t;

// VM_OP_QUERY Request
typedef struct {
    uint32_t region_id;
} vm_req_query_t;

// VM_OP_QUERY Response
typedef struct {
    uint32_t region_id;
    vm_region_state_t state;
    uint64_t vaddr;
    uint64_t size;
    int32_t status;
} vm_resp_query_t;

// VM_OP_FAULT Request (Kernel to VM_Manager)
typedef struct {
    uint32_t aspace_id;
    uint64_t fault_vaddr;
    uint32_t fault_type;
} vm_req_fault_t;

// VM_OP_FAULT Response (VM_Manager to Kernel)
typedef struct {
    int32_t action; // 0 = resolved, -1 = kill
} vm_resp_fault_t;

#ifdef __cplusplus
}
#endif

#endif // BHARAT_UAPI_VM_MANAGER_CONTRACT_H
