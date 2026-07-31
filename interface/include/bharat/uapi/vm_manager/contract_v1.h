#ifndef BHARAT_UAPI_VM_MANAGER_CONTRACT_V1_H
#define BHARAT_UAPI_VM_MANAGER_CONTRACT_V1_H

#include <stdint.h>
#include <bharat/uapi/ipc/contract.h>
#include <uapi/handle/handle.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BH_VM_INTERFACE_VERSION_V1 1U

typedef enum {
    BH_VM_OP_CREATE_SPACE_V1  = 0x10,
    BH_VM_OP_DESTROY_SPACE_V1 = 0x11,
    BH_VM_OP_MAP_V1           = 0x12,
    BH_VM_OP_UNMAP_V1         = 0x13,
    BH_VM_OP_PROTECT_V1       = 0x14,
    BH_VM_OP_QUERY_V1         = 0x15,
} bh_vm_opcode_v1_t;

typedef enum {
    BH_VM_REGION_STATE_FREE_V1 = 0,
    BH_VM_REGION_STATE_RESERVED_V1,
    BH_VM_REGION_STATE_MAPPED_V1,
    BH_VM_REGION_STATE_REVOKED_V1
} bh_vm_region_state_v1_t;

typedef uint64_t bh_object_handle_t;
typedef bh_object_handle_t bh_vm_space_handle_t;
typedef bh_object_handle_t bh_vm_region_handle_t;

// CREATE_SPACE_V1
typedef struct {
    uint32_t abi_version;
    uint32_t struct_size;
    uint32_t memory_profile; // mem_profile_t from vm_space.h
    uint32_t timing_class;   // vm_timing_class_t
} bh_vm_create_space_request_v1_t;

typedef struct {
    uint32_t abi_version;
    uint32_t struct_size;
    int32_t status;
    bh_vm_space_handle_t vm_space_handle;
} bh_vm_create_space_response_v1_t;

// DESTROY_SPACE_V1
typedef struct {
    uint32_t abi_version;
    uint32_t struct_size;
    bh_vm_space_handle_t vm_space_handle;
} bh_vm_destroy_space_request_v1_t;

typedef struct {
    uint32_t abi_version;
    uint32_t struct_size;
    int32_t status;
} bh_vm_destroy_space_response_v1_t;

// MAP_V1
typedef struct {
    uint32_t abi_version;
    uint32_t struct_size;
    bh_vm_space_handle_t vm_space_handle;
    uint64_t vaddr;
    uint64_t size;
    uint64_t protection;
    uint64_t memory_type;
} bh_vm_map_request_v1_t;

typedef struct {
    uint32_t abi_version;
    uint32_t struct_size;
    int32_t status;
    bh_vm_region_handle_t region_handle;
} bh_vm_map_response_v1_t;

// UNMAP_V1
typedef struct {
    uint32_t abi_version;
    uint32_t struct_size;
    bh_vm_space_handle_t vm_space_handle;
    bh_vm_region_handle_t region_handle;
} bh_vm_unmap_request_v1_t;

typedef struct {
    uint32_t abi_version;
    uint32_t struct_size;
    int32_t status;
} bh_vm_unmap_response_v1_t;

// PROTECT_V1
typedef struct {
    uint32_t abi_version;
    uint32_t struct_size;
    bh_vm_space_handle_t vm_space_handle;
    bh_vm_region_handle_t region_handle;
    uint64_t new_protection;
} bh_vm_protect_request_v1_t;

typedef struct {
    uint32_t abi_version;
    uint32_t struct_size;
    int32_t status;
} bh_vm_protect_response_v1_t;

// QUERY_V1
typedef struct {
    uint32_t abi_version;
    uint32_t struct_size;
    bh_vm_space_handle_t vm_space_handle;
    bh_vm_region_handle_t region_handle;
} bh_vm_query_request_v1_t;

typedef struct {
    uint32_t abi_version;
    uint32_t struct_size;
    int32_t status;
    uint32_t state; // bh_vm_region_state_v1_t
    uint64_t vaddr;
    uint64_t size;
    uint64_t protection;
    uint64_t memory_type;
} bh_vm_query_response_v1_t;

#ifdef __cplusplus
}
#endif

#endif // BHARAT_UAPI_VM_MANAGER_CONTRACT_V1_H
