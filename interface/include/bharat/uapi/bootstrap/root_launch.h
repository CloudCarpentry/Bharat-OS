#ifndef BHARAT_UAPI_BOOTSTRAP_ROOT_LAUNCH_H
#define BHARAT_UAPI_BOOTSTRAP_ROOT_LAUNCH_H

#include <stddef.h>
#include <stdint.h>
#include <bharat/uapi/bootstrap/runtime_model.h>

#define BH_ROOT_LAUNCH_ABI_VERSION 0x00010000U
#define BH_USER_STARTUP_FLAG_ROOT_LAUNCH_EXTENSION (1U << 0)

/*
 * Immutable, by-value policy supplied only to the initial userspace task.
 * Version 1 is stored immediately after bharat_user_startup_t when the startup
 * flag above is set. This extends, rather than changes, the generic startup ABI.
 */
typedef struct bh_root_launch_info {
    uint32_t version;
    uint32_t size;
    bh_userspace_runtime_model_t runtime_model;
    uint32_t flags;
    uint32_t root_module_kind;
    uint32_t root_module_id;
    uint64_t boot_session_id;
    uint64_t bundle_manifest_id;
    uint64_t reserved[3];
} bh_root_launch_info_t;

#if defined(__cplusplus)
#define BH_ROOT_LAUNCH_STATIC_ASSERT static_assert
#else
#define BH_ROOT_LAUNCH_STATIC_ASSERT _Static_assert
#endif

BH_ROOT_LAUNCH_STATIC_ASSERT(sizeof(bh_userspace_runtime_model_t) == sizeof(uint32_t),
               "runtime model must have a fixed 32-bit wire representation");
BH_ROOT_LAUNCH_STATIC_ASSERT(sizeof(bh_root_launch_info_t) == 64,
               "root launch ABI layout changed");
BH_ROOT_LAUNCH_STATIC_ASSERT(offsetof(bh_root_launch_info_t, runtime_model) == 8,
               "root launch runtime-model offset changed");
BH_ROOT_LAUNCH_STATIC_ASSERT(offsetof(bh_root_launch_info_t, boot_session_id) == 24,
               "root launch boot-session offset changed");

#undef BH_ROOT_LAUNCH_STATIC_ASSERT

#endif /* BHARAT_UAPI_BOOTSTRAP_ROOT_LAUNCH_H */
