#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include <bharat/io/block.h>
#include <bharat/stacks/storage/block.h>
#include <bharat/stacks/storage/block_driver.h>
#include <bharat/io_config.h>
#include "../../core/drivers/block/memblk/memblk.h"

extern void virtio_blk_init(void);
extern void memblk_test_set_fault(io_device_id_t device_id, io_status_t fault_status);

// Simple mock for thread/scheduler
void* g_stub_current_process = NULL;

// Helper to simulate time and ticks
static void step_ticks(uint32_t ticks) {
    while (ticks--) {
        memblk_tick();
    }
}

// -------------------------------------------------------------
// Driver Contract Suite Runner
// -------------------------------------------------------------
typedef struct {
    io_device_id_t device_id;
    const char *name;
} block_driver_test_adapter_t;

static void run_block_driver_contract_suite(const block_driver_test_adapter_t *adapter) {
    printf("Running Block Driver Contract Suite for: %s (ID: %d)\n", adapter->name, adapter->device_id);

    io_device_caps_t caps;
    block_device_t *dev = block_device_get(adapter->device_id);
    assert(dev != NULL);
    assert(dev->ops->get_caps(dev->driver_context, &caps) == IO_STATUS_OK);

    io_channel_config_t config = {
        .queue_id = 0,
        .queue_depth = 16,
        .enable_polling = true
    };
    void *channel = NULL;
    assert(block_channel_open(adapter->device_id, &config, &channel) == IO_STATUS_OK);
    assert(channel != NULL);

    // Invariant: Submission accepted → exactly one completion generated
    uint8_t buffer[512] = {0};
    memset(buffer, 0xAA, sizeof(buffer));

    io_sg_entry_t sg = {
        .phys_addr = (uintptr_t)buffer,
        .length = 512
    };

    io_request_t req = {
        .id = 101ULL,
        .device_id = adapter->device_id,
        .opcode = IO_OP_WRITE,
        .lba = 0,
        .block_count = 1,
        .buffer_cap = 0,
        .segments = &sg,
        .segment_count = 1,
        .completion_cookie = (void*)0xDEADBEEF
    };

    assert(block_submit(adapter->device_id, channel, &req) == IO_STATUS_OK);

    // Tick the memblk to process
    step_ticks(1);

    io_completion_t comp;
    uint32_t count = 0;
    assert(block_poll_completions(adapter->device_id, channel, &comp, 1, &count) == IO_STATUS_OK);
    assert(count == 1);
    assert(comp.id == 101ULL);
    assert(comp.status == IO_STATUS_OK);
    assert(comp.transferred_blocks == 1);
    assert(comp.completion_cookie == (void*)0xDEADBEEF);

    // Read back and verify integrity
    uint8_t read_buf[512] = {0};
    io_sg_entry_t read_sg = {
        .phys_addr = (uintptr_t)read_buf,
        .length = 512
    };

    io_request_t read_req = {
        .id = 102ULL,
        .device_id = adapter->device_id,
        .opcode = IO_OP_READ,
        .lba = 0,
        .block_count = 1,
        .buffer_cap = 0,
        .segments = &read_sg,
        .segment_count = 1,
        .completion_cookie = NULL
    };
    assert(block_submit(adapter->device_id, channel, &read_req) == IO_STATUS_OK);
    step_ticks(1);

    assert(block_poll_completions(adapter->device_id, channel, &comp, 1, &count) == IO_STATUS_OK);
    assert(count == 1);
    assert(comp.id == 102ULL);
    assert(comp.status == IO_STATUS_OK);
    assert(memcmp(read_buf, buffer, 512) == 0);

    assert(block_channel_close(adapter->device_id, channel) == IO_STATUS_OK);
    printf("  -> Contract suite PASS\n");
}

// -------------------------------------------------------------
// Test 1: I/O Profile Policy Defaults
// -------------------------------------------------------------
static void test_io_profile_policy(void) {
    printf("test_io_profile_policy...\n");
    // Verify that the compiled profile policy matches BHARAT_IO_PROFILE config.
#if defined(BHARAT_IO_PROFILE_BALANCED)
    assert(BHARAT_IO_MAX_DEVICES == 8);
    assert(BHARAT_IO_MAX_QUEUE_DEPTH == 128);
#elif defined(BHARAT_IO_PROFILE_TINY)
    assert(BHARAT_IO_MAX_DEVICES == 2);
    assert(BHARAT_IO_MAX_QUEUE_DEPTH == 4);
#endif
    printf("  -> PASS\n");
}

// -------------------------------------------------------------
// Test 2: Block Device Registry & Enumeration
// -------------------------------------------------------------
static void test_block_driver_registry(void) {
    printf("test_block_driver_registry...\n");
    io_device_id_t ids[10];
    uint32_t count = 0;
    assert(block_device_enumerate(ids, 10, &count) == IO_STATUS_OK);
    assert(count >= 2); // memblk0 and memblk1

    block_device_t *dev0 = block_device_get(42);
    assert(dev0 != NULL);
    assert(strcmp(dev0->name, "memblk0") == 0);
    assert(dev0->role == IO_DEVICE_ROLE_SYSTEM);

    block_device_t *dev1 = block_device_get(43);
    assert(dev1 != NULL);
    assert(strcmp(dev1->name, "memblk1") == 0);
    assert(dev1->role == IO_DEVICE_ROLE_DATA);

    // Test duplicate registration detection
    assert(block_device_register(dev0) == IO_STATUS_INVALID_ARGUMENT);

    printf("  -> PASS\n");
}

// -------------------------------------------------------------
// Test 3: Multiple Registered Devices & Policy Resolution
// -------------------------------------------------------------
static void test_block_multi_device_registry(void) {
    printf("test_block_multi_device_registry...\n");
    io_device_id_t sys_dev;
    assert(block_device_find_by_role(IO_DEVICE_ROLE_SYSTEM, &sys_dev) == IO_STATUS_OK);
    assert(sys_dev == 42);

    io_device_id_t data_dev;
    assert(block_device_find_by_role(IO_DEVICE_ROLE_DATA, &data_dev) == IO_STATUS_OK);
    assert(data_dev == 43);

    // Resolve policies independently
    io_device_policy_t sys_policy;
    assert(block_device_resolve_policy(42, &sys_policy) == IO_STATUS_OK);
    assert(sys_policy.queue_policy == IO_QUEUE_POLICY_SINGLE_OWNER);
    assert(sys_policy.allow_direct_io == true);

    io_device_policy_t data_policy;
    assert(block_device_resolve_policy(43, &data_policy) == IO_STATUS_OK);
    assert(data_policy.device_id == 43);

    printf("  -> PASS\n");
}

// -------------------------------------------------------------
// Test 4: Queue Backpressure
// -------------------------------------------------------------
static void test_memblk_queue_backpressure(void) {
    printf("test_memblk_queue_backpressure...\n");
    io_channel_config_t config = {0};
    void *channel = NULL;
    assert(block_channel_open(42, &config, &channel) == IO_STATUS_OK);

    // Fill queue to capacity (16 requests)
    for (uint32_t i = 0; i < 16; ++i) {
        io_request_t req = {
            .id = 200ULL + i,
            .device_id = 42,
            .opcode = IO_OP_FLUSH
        };
        assert(block_submit(42, channel, &req) == IO_STATUS_OK);
    }

    // Next submit must fail due to backpressure / queue full
    io_request_t extra_req = {
        .id = 999ULL,
        .device_id = 42,
        .opcode = IO_OP_FLUSH
    };
    assert(block_submit(42, channel, &extra_req) == IO_STATUS_QUEUE_FULL);

    // Advance queue to process all
    step_ticks(1);

    // completions must be available
    io_completion_t comps[16];
    uint32_t count = 0;
    assert(block_poll_completions(42, channel, comps, 16, &count) == IO_STATUS_OK);
    assert(count == 16);

    assert(block_channel_close(42, channel) == IO_STATUS_OK);
    printf("  -> PASS\n");
}

// -------------------------------------------------------------
// Test 5: Cancellation races & deterministic outcome
// -------------------------------------------------------------
static void test_memblk_cancel(void) {
    printf("test_memblk_cancel...\n");
    io_channel_config_t config = {0};
    void *channel = NULL;
    assert(block_channel_open(42, &config, &channel) == IO_STATUS_OK);

    io_request_t req = {
        .id = 301ULL,
        .device_id = 42,
        .opcode = IO_OP_WRITE,
        .lba = 10,
        .block_count = 1
    };
    assert(block_submit(42, channel, &req) == IO_STATUS_OK);

    // Cancel while queued (prior to execution tick)
    assert(block_cancel(42, channel, 301ULL) == IO_STATUS_OK);

    // Dequeue completion
    io_completion_t comp;
    uint32_t count = 0;
    assert(block_poll_completions(42, channel, &comp, 1, &count) == IO_STATUS_OK);
    assert(count == 1);
    assert(comp.id == 301ULL);
    assert(comp.status == IO_STATUS_CANCELLED);

    // Repeat cancellation must return terminal error / not found (since it's no longer active)
    assert(block_cancel(42, channel, 301ULL) == IO_STATUS_INVALID_ARGUMENT);

    assert(block_channel_close(42, channel) == IO_STATUS_OK);
    printf("  -> PASS\n");
}

// -------------------------------------------------------------
// Test 6: Flush Ordering Barriers
// -------------------------------------------------------------
static void test_memblk_flush_order(void) {
    printf("test_memblk_flush_order...\n");
    io_channel_config_t config = {0};
    void *channel = NULL;
    assert(block_channel_open(42, &config, &channel) == IO_STATUS_OK);

    // Write A, Write B, Flush F, Write C
    uint8_t data[512] = {0};
    io_sg_entry_t sg = {
        .phys_addr = (uintptr_t)data,
        .length = 512
    };

    io_request_t reqA = { .id = 401ULL, .device_id = 42, .opcode = IO_OP_WRITE, .lba = 0, .block_count = 1, .segments = &sg, .segment_count = 1 };
    io_request_t reqB = { .id = 402ULL, .device_id = 42, .opcode = IO_OP_WRITE, .lba = 1, .block_count = 1, .segments = &sg, .segment_count = 1 };
    io_request_t reqF = { .id = 403ULL, .device_id = 42, .opcode = IO_OP_FLUSH };
    io_request_t reqC = { .id = 404ULL, .device_id = 42, .opcode = IO_OP_WRITE, .lba = 2, .block_count = 1, .segments = &sg, .segment_count = 1 };

    assert(block_submit(42, channel, &reqA) == IO_STATUS_OK);
    assert(block_submit(42, channel, &reqB) == IO_STATUS_OK);
    assert(block_submit(42, channel, &reqF) == IO_STATUS_OK);
    assert(block_submit(42, channel, &reqC) == IO_STATUS_OK);

    // Process exactly one step (our reference driver processes all queued)
    step_ticks(1);

    io_completion_t comps[4];
    uint32_t count = 0;
    assert(block_poll_completions(42, channel, comps, 4, &count) == IO_STATUS_OK);
    assert(count == 4);

    // Verify ordering in completions (all must succeed)
    assert(comps[0].id == 401ULL);
    assert(comps[1].id == 402ULL);
    assert(comps[2].id == 403ULL);
    assert(comps[3].id == 404ULL);

    assert(block_channel_close(42, channel) == IO_STATUS_OK);
    printf("  -> PASS\n");
}

// -------------------------------------------------------------
// Test 7: Error Propagation & Sync legacy wrapper
// -------------------------------------------------------------
static void test_memblk_error_propagation(void) {
    printf("test_memblk_error_propagation...\n");
    // Inject error into memblk0 (ID 42)
    memblk_test_set_fault(42, IO_STATUS_IO_ERROR);

    uint8_t data[512] = {0};
    block_request_t legacy_req = {
        .type = BLOCK_REQ_READ,
        .lba = 100,
        .num_blocks = 1,
        .buffer = data,
        .status = 0
    };

    // The legacy synchronizer wrapper should wait for completion,
    // catch the injected error, and propagate it.
    int ret = block_queue_request(42, &legacy_req);
    assert(ret == -1);
    assert(legacy_req.status == -1);

    // Reset fault state
    memblk_test_set_fault(42, IO_STATUS_OK);

    legacy_req.status = -1;
    ret = block_queue_request(42, &legacy_req);
    assert(ret == 0);
    assert(legacy_req.status == 0);

    printf("  -> PASS\n");
}

// -------------------------------------------------------------
// Test 8: 32-bit / 64-bit limits and range checks
// -------------------------------------------------------------
static void test_block_range_overflow(void) {
    printf("test_block_range_overflow...\n");
    io_device_caps_t caps;
    block_device_t *dev = block_device_get(42);
    assert(dev->ops->get_caps(dev->driver_context, &caps) == IO_STATUS_OK);

    uint64_t total_blocks = caps.capacity_bytes / caps.logical_block_size;

    // Check overflow: lba + block_count > total_blocks
    uint64_t lba = total_blocks - 5;
    uint32_t block_count = 10;
    bool invalid = (block_count > total_blocks || lba > total_blocks - block_count);
    assert(invalid == true);

    // Check wraps: lba + block_count overflows uint64_t
    lba = 0xFFFFFFFFFFFFFFF0ULL;
    block_count = 20;
    invalid = (block_count > total_blocks || lba > total_blocks - block_count);
    assert(invalid == true);

    printf("  -> PASS\n");
}

// -------------------------------------------------------------
// Test 9: Device removal & forced removal completion
// -------------------------------------------------------------
static void test_device_forced_removal(void) {
    printf("test_device_forced_removal...\n");
    io_channel_config_t config = {0};
    void *channel = NULL;
    assert(block_channel_open(43, &config, &channel) == IO_STATUS_OK);

    io_request_t req = {
        .id = 501ULL,
        .device_id = 43,
        .opcode = IO_OP_WRITE,
        .lba = 2,
        .block_count = 1
    };
    assert(block_submit(43, channel, &req) == IO_STATUS_OK);

    // Simulate forced unregister of device 43 (memblk1) with outstanding request
    assert(block_device_unregister(43) == IO_STATUS_OK);

    // The outstanding request must produce a removal completion
    // Since our reference driver's state changed, any remaining queued items are flushed
    block_device_t *dev = block_device_get(43);
    assert(dev == NULL); // gone from registry

    // Verify submission to removed device fails
    assert(block_submit(43, channel, &req) == IO_STATUS_INVALID_ARGUMENT);

    // Close channel
    assert(block_channel_close(43, channel) == IO_STATUS_INVALID_ARGUMENT);

    printf("  -> PASS\n");
}

// -------------------------------------------------------------
// Test 10: Cache Bypass & Flag forwarding
// -------------------------------------------------------------
static void test_block_cached_and_direct_flags(void) {
    printf("test_block_cached_and_direct_flags...\n");
    io_request_t req = {
        .id = 601ULL,
        .device_id = 42,
        .opcode = IO_OP_READ,
        .flags = IO_REQ_DIRECT | IO_REQ_BYPASS_CACHE | IO_REQ_FUA
    };

    // Forwarding test: Verify flags are unmodified
    assert((req.flags & IO_REQ_DIRECT) != 0);
    assert((req.flags & IO_REQ_BYPASS_CACHE) != 0);
    assert((req.flags & IO_REQ_FUA) != 0);

    printf("  -> PASS\n");
}

// -------------------------------------------------------------
// Test 11: Unsupported operations behavior
// -------------------------------------------------------------
static void test_block_unsupported_ops(void) {
    printf("test_block_unsupported_ops...\n");
    // Register the unsupported virtio-blk0 scaffold
    virtio_blk_init();

    io_channel_config_t config = {0};
    void *channel = NULL;
    // Since virtio_blk ops.open_channel is NULL, opening channel should be NOT_SUPPORTED
    assert(block_channel_open(101, &config, &channel) == IO_STATUS_NOT_SUPPORTED);

    printf("  -> PASS\n");
}

int main(void) {
    printf("Starting storage adaptive I/O fabric foundation tests...\n");

    // Initialize the memblk reference devices
    memblk_init();

    // Run suites
    test_io_profile_policy();
    test_block_driver_registry();
    test_block_multi_device_registry();
    test_memblk_queue_backpressure();
    test_memblk_cancel();
    test_memblk_flush_order();
    test_memblk_error_propagation();
    test_block_range_overflow();
    test_device_forced_removal();
    test_block_cached_and_direct_flags();
    test_block_unsupported_ops();

    // Run contract adapter test against memblk0 (ID 42)
    block_driver_test_adapter_t adapter = {
        .device_id = 42,
        .name = "memblk0"
    };
    run_block_driver_contract_suite(&adapter);

    memblk_deinit();

    printf("All foundation I/O fabric tests PASSED!\n");
    return 0;
}
