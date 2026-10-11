#include <bharat/compute/tensor.h>
#include <bharat/memory/hmem.h>
#include <bharat/core/status.h>
#include <bharat/syscall.h>

bh_status_t bh_hmem_create(const bharat_hmem_desc_v1_t *desc, bharat_hmem_handle_t *out_handle) {
    if (!desc || !out_handle) return BH_ERR_INVALID_ARG;
    if (desc->version != BH_HMEM_DESC_VERSION_1) return BH_ERR_UNSUPPORTED;

    bh_hmem_create_args_t args = {
        .struct_size = sizeof(bh_hmem_create_args_t),
        .desc = *desc,
        .rights = BH_HMEM_RIGHT_ALL,
        .out_handle = 0
    };

    int32_t syscall_res = bh_syscall1(BH_SYS_HMEM_CREATE, (uintptr_t)&args);
    if (syscall_res < 0) return BH_ERR_PLATFORM;

    *out_handle = args.out_handle;
    return BH_OK;
}

bh_status_t bh_hmem_destroy(bharat_hmem_handle_t handle) {
    bh_hmem_query_args_t args = {
        .struct_size = sizeof(bh_hmem_query_args_t),
        .hmem_cap = handle
    };
    int32_t syscall_res = bh_syscall1(BH_SYS_HMEM_DESTROY, (uintptr_t)&args);
    return syscall_res < 0 ? BH_ERR_PLATFORM : BH_OK;
}

bh_status_t bh_hmem_query(bharat_hmem_handle_t handle, bharat_hmem_info_v1_t *out_info) {
    if (!out_info) return BH_ERR_INVALID_ARG;
    bh_hmem_query_args_t args = {
        .struct_size = sizeof(bh_hmem_query_args_t),
        .hmem_cap = handle
    };
    int32_t syscall_res = bh_syscall1(BH_SYS_HMEM_QUERY, (uintptr_t)&args);
    if (syscall_res < 0) return BH_ERR_PLATFORM;

    *out_info = args.out_info;
    return BH_OK;
}

bh_status_t bh_hmem_map_cpu(bharat_hmem_handle_t handle, uint32_t access_flags, void **out_addr) {
    if (!out_addr) return BH_ERR_INVALID_ARG;
    bh_hmem_map_cpu_args_t args = {
        .struct_size = sizeof(bh_hmem_map_cpu_args_t),
        .hmem_cap = handle,
        .access = access_flags,
        .reserved = 0,
        .out_addr = 0
    };
    int32_t syscall_res = bh_syscall1(BH_SYS_HMEM_MAP_CPU, (uintptr_t)&args);
    if (syscall_res < 0) {
        // Important Phase 0 requirement: propagate UNSUPPORTED explicitly so apps can fail cleanly.
        // We assume syscall returns mapped error codes, but for now we just return unsupported if negative.
        return BH_ERR_UNSUPPORTED;
    }

    *out_addr = (void*)(uintptr_t)args.out_addr;
    return BH_OK;
}

bh_status_t bh_hmem_unmap_cpu(bharat_hmem_handle_t handle, void *addr) {
    bh_hmem_unmap_cpu_args_t args = {
        .struct_size = sizeof(bh_hmem_unmap_cpu_args_t),
        .hmem_cap = handle
    };
    int32_t syscall_res = bh_syscall1(BH_SYS_HMEM_UNMAP_CPU, (uintptr_t)&args);
    return syscall_res < 0 ? BH_ERR_UNSUPPORTED : BH_OK;
}

bh_status_t bh_hmem_sync_for_cpu(bharat_hmem_handle_t handle, uint64_t offset, uint64_t size) {
    bh_hmem_sync_args_t args = {
        .struct_size = sizeof(bh_hmem_sync_args_t),
        .hmem_cap = handle,
        .offset = offset,
        .length = size
    };
    int32_t syscall_res = bh_syscall1(BH_SYS_HMEM_SYNC_FOR_CPU, (uintptr_t)&args);
    return syscall_res < 0 ? BH_ERR_PLATFORM : BH_OK;
}

bh_status_t bh_hmem_sync_for_device(bharat_hmem_handle_t handle, uint64_t offset, uint64_t size) {
    bh_hmem_sync_args_t args = {
        .struct_size = sizeof(bh_hmem_sync_args_t),
        .hmem_cap = handle,
        .offset = offset,
        .length = size
    };
    int32_t syscall_res = bh_syscall1(BH_SYS_HMEM_SYNC_FOR_DEVICE, (uintptr_t)&args);
    return syscall_res < 0 ? BH_ERR_PLATFORM : BH_OK;
}
