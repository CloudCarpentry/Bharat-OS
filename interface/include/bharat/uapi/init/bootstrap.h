#ifndef BHARAT_UAPI_INIT_BOOTSTRAP_H
#define BHARAT_UAPI_INIT_BOOTSTRAP_H

#include <stdint.h>
#include <bharat/uapi/abi_types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BH_BOOTSTRAP_SCOPE_SYSTEM = 0,
    BH_BOOTSTRAP_SCOPE_KERNEL_INSTANCE,
    BH_BOOTSTRAP_SCOPE_CORE
} bh_bootstrap_scope_t;

typedef struct bharat_bootstrap_info {
    uint32_t abi_version;
    uint32_t struct_size;

    uint64_t boot_session_id;

    uint32_t kernel_instance_id;
    uint32_t home_core_id;

    uint64_t online_core_mask;
    uint64_t available_kernel_mask;

    bharat_handle_t self_process_cap;
    bharat_handle_t bootstrap_cap;

    bharat_handle_t system_control_endpoint;
    bharat_handle_t local_kernel_endpoint;

    uint32_t boot_mode;
    uint32_t security_state;

    uint64_t flags;
} bharat_bootstrap_info_t;

typedef struct bharat_user_startup {
    uint32_t abi_version;
    uint32_t struct_size;

    uint32_t argc;
    uint32_t flags;

    uintptr_t argv;
    uintptr_t envp;

    bharat_bootstrap_info_t bootstrap;
} bharat_user_startup_t;

#ifdef __cplusplus
}
#endif

#endif // BHARAT_UAPI_INIT_BOOTSTRAP_H
