#include <stdint.h>
#include <stddef.h>

#include "urpc.h"
#include "bharat/stacks/storage/block.h"
#include "bharat/stacks/storage/cache/cache.h"
#include "bharat/stacks/storage/profile.h"

// Reference happy path execution
extern int virtio_blk_submit_request(void* sg_list, uint32_t num_sgs);
extern void virtio_blk_init(void);

static storage_app_profile_t fs_select_profile(void) {
#if defined(CONFIG_PROFILE_AUTOMOBILE) || defined(CONFIG_PROFILE_DRONE)
    return STORAGE_APP_PROFILE_RT;
#elif defined(CONFIG_PROFILE_DATACENTER)
    return STORAGE_APP_PROFILE_DATACENTER;
#else
    return STORAGE_APP_PROFILE_EDGE;
#endif
}

static storage_hw_arch_t fs_select_arch(void) {
#if defined(__x86_64__) || defined(_M_X64)
    return STORAGE_HW_ARCH_X86_64;
#elif defined(__aarch64__)
    return STORAGE_HW_ARCH_ARM64;
#elif defined(__arm__)
    return STORAGE_HW_ARCH_ARM32;
#elif defined(__riscv) && (__riscv_xlen == 64)
    return STORAGE_HW_ARCH_RISCV64;
#elif defined(__riscv)
    return STORAGE_HW_ARCH_RISCV32;
#else
    return STORAGE_HW_ARCH_UNKNOWN;
#endif
}

static int fs_storage_stack_init(uint32_t device_id) {
    block_device_info_t info;
    storage_profile_config_t cfg;

    if (block_get_info(device_id, &info) != 0) {
        return -1;
    }

    if (storage_profile_resolve(fs_select_profile(),
                                (uint64_t)info.block_size * info.total_blocks,
                                fs_select_arch(),
                                &cfg) != 0) {
        return -1;
    }

    return cache_init_with_profile(&cfg);
}

// Simulated capability IPC server handler
void handle_fs_urpc_request(void* msg) {
    (void)msg; // Suppress unused
    // App -> Service (VFS)
    // Here VFS would decode `msg`, validate capability, check mount, and open/read file.

    // Service -> Stack (Block)
    block_request_t breq = {
        .type = BLOCK_REQ_READ,
        .lba = 0,
        .num_blocks = 1,
        .buffer = NULL, // Provide simulated DMA buffer here
        .status = -1
    };

    block_queue_request(0, &breq); // Stacks block abstraction
}

int main(void) {
    // 1. Initialize drivers
    virtio_blk_init();

    // 2. Initialize cache and storage policy based on profile/device/arch.
    if (fs_storage_stack_init(0) != 0) {
        return -1;
    }

    // 3. Wait for incoming capability requests
    // (Simulate an incoming uRPC request loop)

    return 0;
}
