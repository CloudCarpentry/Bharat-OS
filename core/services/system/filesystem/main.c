#include <stdint.h>
#include <stddef.h>

#include "urpc.h"
#include "bharat/stacks/storage/block.h"
#include "bharat/stacks/storage/cache/cache.h"
#include "bharat/stacks/storage/profile.h"
#include "fs_arch_profile.h"
#include <bharat/io_config.h>
#include <stdlib.h>

extern void block_stacks_init(void);

// Userspace compatibility wrappers for transitional code
void* kmalloc(size_t size) {
    return malloc(size);
}

void kfree(void* ptr) {
    free(ptr);
}

static io_device_id_t g_system_device_id = 42; // default fallback

static storage_app_profile_t fs_select_profile(void) {
#if defined(BHARAT_PROFILE_AUTOMOTIVE_ECU) || defined(BHARAT_PROFILE_DRONE) || defined(BHARAT_PROFILE_RTOS)
    return STORAGE_APP_PROFILE_RT;
#elif defined(BHARAT_PROFILE_DATACENTER)
    return STORAGE_APP_PROFILE_DATACENTER;
#else
    return STORAGE_APP_PROFILE_EDGE;
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

    block_queue_request(g_system_device_id, &breq); // Stacks block abstraction
}

int main(void) {
    // 1. Initialize storage stack and drivers
    block_stacks_init();

    // 2. Discover default system storage device
    if (block_device_find_by_role(IO_DEVICE_ROLE_SYSTEM, &g_system_device_id) != IO_STATUS_OK) {
        g_system_device_id = 42; // default fallback (memblk0)
    }

    // 3. Initialize cache and storage policy based on profile/device/arch.
    if (fs_storage_stack_init(g_system_device_id) != 0) {
        return -1;
    }

    // 4. Wait for incoming capability requests
    // (Simulate an incoming uRPC request loop)

    return 0;
}
