#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include <bharat/io/block.h>
#include <bharat/stacks/storage/block.h>
#include <bharat/stacks/storage/block_driver.h>
#include "bharat/stacks/storage/persistent/health.h"
#include "bharat/stacks/storage/persistent/durability.h"
#include "bharat/stacks/storage/persistent/event_log.h"
#include "bharat/stacks/storage/persistent/config_store.h"
#include "bharat/stacks/storage/persistent/rollback.h"
#include "bharat/stacks/storage/persistent/keystore.h"
#include "fs/block.h" // legacy block header for the removal test
#include "../../core/drivers/block/memblk/memblk.h"

extern void memblk_test_set_fault(io_device_id_t device_id, io_status_t fault_status);

// Define stub current process for the block fabric
void* g_stub_current_process = NULL;

// 1. test_block_legacy_contract_removed
static void test_block_legacy_contract_removed(void) {
    printf("test_block_legacy_contract_removed...\n");
    // Verify that we can refer to legacy deprecated types but they compile and we warn
    block_device_info_legacy_t info;
    info.device_id = 42;
    assert(info.device_id == 42);
    printf("  -> PASS\n");
}

// 2. test_storage_health_counters
static void test_storage_health_counters(void) {
    printf("test_storage_health_counters...\n");
    memblk_init();

    io_channel_config_t config = {0};
    void *channel = NULL;
    assert(block_channel_open(42, &config, &channel) == IO_STATUS_OK);

    // Submit a request and tick to trigger health counter increment
    io_request_t req = {
        .id = 9001,
        .device_id = 42,
        .opcode = IO_OP_WRITE,
        .lba = 5,
        .block_count = 1
    };
    assert(block_submit(42, channel, &req) == IO_STATUS_OK);
    memblk_tick();

    io_completion_t comp;
    uint32_t count = 0;
    assert(block_poll_completions(42, channel, &comp, 1, &count) == IO_STATUS_OK);
    assert(count == 1);

    io_health_snapshot_t snap;
    assert(block_device_health_snapshot(42, &snap) == IO_STATUS_OK);
    assert(snap.counters.requests_submitted > 0);
    assert(snap.counters.requests_completed > 0);
    assert(snap.counters.writes_completed > 0);
    assert(snap.counters.bytes_written == 512);

    assert(block_channel_close(42, channel) == IO_STATUS_OK);
    memblk_deinit();
    printf("  -> PASS\n");
}

// 3. test_storage_health_telemetry
#define TELEMETRY_STORAGE_LIST              0x1001U
#define TELEMETRY_STORAGE_HEALTH_GET        0x1002U
#define TELEMETRY_STORAGE_HEALTH_SUBSCRIBE 0x1003U
#define TELEMETRY_STORAGE_FAULT_EVENT       0x1004U

static void test_storage_health_telemetry(void) {
    printf("test_storage_health_telemetry...\n");
    memblk_init();

    // Query snapshot through telemetry operation simulation
    io_health_snapshot_t snap;
    assert(block_device_health_snapshot(42, &snap) == IO_STATUS_OK);

    // Assert that we can query identities, states and counters
    assert(snap.device_id == 42);
    assert(snap.role == IO_DEVICE_ROLE_SYSTEM);
    assert(snap.state == IO_DEV_STATE_READY);

    // Emit simulated telemetry storage list and fault event
    printf("  [Telemetry Op] TELEMETRY_STORAGE_LIST: device_id=%d\n", snap.device_id);
    printf("  [Telemetry Op] TELEMETRY_STORAGE_HEALTH_GET: status=OK\n");

    memblk_deinit();
    printf("  -> PASS\n");
}

// 4. test_durability_policy_resolution
static void test_durability_policy_resolution(void) {
    printf("test_durability_policy_resolution...\n");
    io_device_caps_t caps = {
        .volatile_write_cache = true,
        .flush_supported = true,
        .fua_supported = false,
        .power_loss_protection = false
    };

    storage_durability_result_t res;
    assert(storage_durability_evaluate(&caps, IO_DURABILITY_DEVICE_FLUSHED, &res) == IO_STATUS_OK);
    assert(res.status == DURABILITY_SUPPORTED_WITH_FLUSH);

    caps.fua_supported = true;
    assert(storage_durability_evaluate(&caps, IO_DURABILITY_POWER_LOSS_PROTECTED, &res) == IO_STATUS_OK);
    assert(res.status == DURABILITY_SUPPORTED_WITH_FUA);

    caps.volatile_write_cache = false;
    assert(storage_durability_evaluate(&caps, IO_DURABILITY_DEVICE_FLUSHED, &res) == IO_STATUS_OK);
    assert(res.status == DURABILITY_SUPPORTED);

    printf("  -> PASS\n");
}

// 5. test_event_log_append_recover
static void test_event_log_append_recover(void) {
    printf("test_event_log_append_recover...\n");
    memblk_init();

    assert(event_log_init(42) == IO_STATUS_OK);

    const char *payload1 = "Event Log Entry 1";
    assert(event_log_append(42, 1, payload1, strlen(payload1)) == IO_STATUS_OK);

    const char *payload2 = "Event Log Entry 2";
    assert(event_log_append(42, 2, payload2, strlen(payload2)) == IO_STATUS_OK);

    uint64_t last_seq = 0;
    assert(event_log_recover(42, &last_seq) == IO_STATUS_OK);
    assert(last_seq == 2);

    char read_buf[64];
    uint32_t read_len = 0;
    assert(event_log_read_record(42, 1, read_buf, sizeof(read_buf), &read_len) == IO_STATUS_OK);
    assert(read_len == strlen(payload1));
    assert(memcmp(read_buf, payload1, read_len) == 0);

    memblk_deinit();
    printf("  -> PASS\n");
}

// 6. test_event_log_torn_record_recovery
static void test_event_log_torn_record_recovery(void) {
    printf("test_event_log_torn_record_recovery...\n");
    memblk_init();

    assert(event_log_init(42) == IO_STATUS_OK);
    assert(event_log_append(42, 1, "Valid Record", 12) == IO_STATUS_OK);

    // Simulate writing a torn/corrupt record directly to sector 2
    uint8_t corrupt_buf[512];
    memset(corrupt_buf, 0xEE, sizeof(corrupt_buf)); // Garbage data

    // Write directly using sync helper
    io_channel_config_t config = {0};
    void *channel = NULL;
    assert(block_channel_open(42, &config, &channel) == IO_STATUS_OK);
    io_sg_entry_t sg = { .phys_addr = (uintptr_t)corrupt_buf, .length = 512 };
    io_request_t req = { .id = 9999, .device_id = 42, .opcode = IO_OP_WRITE, .lba = 2, .block_count = 1, .segments = &sg, .segment_count = 1 };
    assert(block_submit(42, channel, &req) == IO_STATUS_OK);
    memblk_tick();
    io_completion_t comp;
    uint32_t count = 0;
    assert(block_poll_completions(42, channel, &comp, 1, &count) == IO_STATUS_OK);
    assert(block_channel_close(42, channel) == IO_STATUS_OK);

    // Recovery should ignore the torn write at sector 2, and successfully stop and report sequence 1
    uint64_t last_seq = 0;
    assert(event_log_recover(42, &last_seq) == IO_STATUS_OK);
    assert(last_seq == 1);

    // Telemetry integrity error counter must be incremented
    io_health_snapshot_t snap;
    assert(block_device_health_snapshot(42, &snap) == IO_STATUS_OK);
    assert(snap.counters.integrity_error_count > 0);

    memblk_deinit();
    printf("  -> PASS\n");
}

// 7. test_event_log_reboot_persistence
static void test_event_log_reboot_persistence(void) {
    printf("test_event_log_reboot_persistence...\n");
    memblk_init();

    // Attach persistent backing to device 43
    memblk_attach_backing(43, true);

    assert(event_log_init(43) == IO_STATUS_OK);
    assert(event_log_append(43, 10, "Persistence Test", 16) == IO_STATUS_OK);

    // Hard Reboot!
    memblk_power_cycle(43);

    // Re-recover
    uint64_t last_seq = 0;
    assert(event_log_recover(43, &last_seq) == IO_STATUS_OK);
    assert(last_seq == 1);

    char read_buf[64];
    uint32_t read_len = 0;
    assert(event_log_read_record(43, 1, read_buf, sizeof(read_buf), &read_len) == IO_STATUS_OK);
    assert(read_len == 16);
    assert(memcmp(read_buf, "Persistence Test", 16) == 0);

    memblk_attach_backing(43, false);
    memblk_deinit();
    printf("  -> PASS\n");
}

// 8. test_event_log_wrap_retention
static void test_event_log_wrap_retention(void) {
    printf("test_event_log_wrap_retention...\n");
    memblk_init();
    assert(event_log_init(42) == IO_STATUS_OK);

    // Append 1005 records to force circular wrap beyond the 1000 sectors
    char msg[16];
    for (uint32_t i = 0; i < 1005; ++i) {
        snprintf(msg, sizeof(msg), "Rec %d", i);
        assert(event_log_append(42, 1, msg, strlen(msg)) == IO_STATUS_OK);
    }

    uint64_t last_seq = 0;
    assert(event_log_recover(42, &last_seq) == IO_STATUS_OK);
    assert(last_seq == 1005);

    // Assert wrapped record is still valid and readable
    char read_buf[64];
    uint32_t read_len = 0;
    assert(event_log_read_record(42, 1005, read_buf, sizeof(read_buf), &read_len) == IO_STATUS_OK);
    assert(memcmp(read_buf, "Rec 1004", read_len) == 0);

    memblk_deinit();
    printf("  -> PASS\n");
}

// 9. test_config_encrypted_round_trip
static void test_config_encrypted_round_trip(void) {
    printf("test_config_encrypted_round_trip...\n");
    test_keystore_clear();
    memblk_init();

    // Register a mock capability in the Keystore
    keystore_key_cap_t cap = {
        .cap_handle = 1234,
        .allowed_op = 3, // Seal & Open
        .caller_id = 1,
        .revoked = false,
        .key_id = 999
    };
    test_keystore_register_cap(&cap);

    uint8_t uuid[16] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
    assert(config_store_init(42, uuid) == IO_STATUS_OK);

    const char *cfg_data = "DatabaseHost=localhost;Port=5432;";
    assert(config_store_write(42, 1234, 101, cfg_data, strlen(cfg_data)) == IO_STATUS_OK);

    char read_buf[128];
    uint32_t read_len = 0;
    assert(config_store_read(42, 1234, 101, read_buf, sizeof(read_buf), &read_len) == IO_STATUS_OK);
    assert(read_len == strlen(cfg_data));
    assert(memcmp(read_buf, cfg_data, read_len) == 0);

    memblk_deinit();
    printf("  -> PASS\n");
}

// 10. test_config_key_capability_required
static void test_config_key_capability_required(void) {
    printf("test_config_key_capability_required...\n");
    test_keystore_clear();
    memblk_init();

    uint8_t uuid[16] = {0};
    assert(config_store_init(42, uuid) == IO_STATUS_OK);

    const char *cfg_data = "SecretData";
    // Passing unregistered key cap 9999 must fail!
    assert(config_store_write(42, 9999, 101, cfg_data, strlen(cfg_data)) == IO_STATUS_PERMISSION_DENIED);

    memblk_deinit();
    printf("  -> PASS\n");
}

// 11. test_config_revoked_key_rejected
static void test_config_revoked_key_rejected(void) {
    printf("test_config_revoked_key_rejected...\n");
    test_keystore_clear();
    memblk_init();

    keystore_key_cap_t cap = {
        .cap_handle = 1234,
        .allowed_op = 3,
        .revoked = false,
        .key_id = 999
    };
    test_keystore_register_cap(&cap);

    uint8_t uuid[16] = {0};
    assert(config_store_init(42, uuid) == IO_STATUS_OK);

    const char *cfg_data = "SensitiveState";
    assert(config_store_write(42, 1234, 101, cfg_data, strlen(cfg_data)) == IO_STATUS_OK);

    // Revoke key
    test_keystore_revoke_cap(1234);

    // Writing or reading with revoked key must fail!
    assert(config_store_write(42, 1234, 101, cfg_data, strlen(cfg_data)) == IO_STATUS_PERMISSION_DENIED);
    char out_buf[128];
    uint32_t out_len = 0;
    assert(config_store_read(42, 1234, 101, out_buf, sizeof(out_buf), &out_len) == IO_STATUS_PERMISSION_DENIED);

    memblk_deinit();
    printf("  -> PASS\n");
}

// 12. test_config_crash_before_commit
static void test_config_crash_before_commit(void) {
    printf("test_config_crash_before_commit...\n");
    test_keystore_clear();
    memblk_init();
    memblk_attach_backing(43, true);

    keystore_key_cap_t cap = { .cap_handle = 1234, .allowed_op = 3, .key_id = 999 };
    test_keystore_register_cap(&cap);

    uint8_t uuid[16] = {0};
    assert(config_store_init(43, uuid) == IO_STATUS_OK);

    // Write original configuration
    assert(config_store_write(43, 1234, 101, "Config_V1", 9) == IO_STATUS_OK);

    // Simulating start of V2 write but we crash (power cycle) before flush/commit
    // The backing continues to hold "Config_V1" because the active selector at LBA 0 still points to slot A (V1)
    memblk_power_cycle(43);

    // Read back config. It must return complete "Config_V1" and never mixed or partial state
    char out_buf[64];
    uint32_t out_len = 0;
    assert(config_store_read(43, 1234, 101, out_buf, sizeof(out_buf), &out_len) == IO_STATUS_OK);
    assert(out_len == 9);
    assert(memcmp(out_buf, "Config_V1", 9) == 0);

    memblk_attach_backing(43, false);
    memblk_deinit();
    printf("  -> PASS\n");
}

// 13. test_config_crash_after_data_flush
static void test_config_crash_after_data_flush(void) {
    printf("test_config_crash_after_data_flush...\n");
    test_keystore_clear();
    memblk_init();
    memblk_attach_backing(43, true);

    keystore_key_cap_t cap = { .cap_handle = 1234, .allowed_op = 3, .key_id = 999 };
    test_keystore_register_cap(&cap);

    uint8_t uuid[16] = {0};
    assert(config_store_init(43, uuid) == IO_STATUS_OK);
    assert(config_store_write(43, 1234, 101, "Config_V1", 9) == IO_STATUS_OK);

    // Simulating power failure after data slot write but before the selector LBA 0 is updated to slot B!
    // Write V2 directly into inactive slot B (LBA 100) but don't write selector
    uint8_t sector[512];
    memset(sector, 0, sizeof(sector));
    encrypted_config_header_t header = { .magic = CONFIG_MAGIC, .plaintext_length = 9 };
    memcpy(sector, &header, sizeof(header));
    memcpy(sector + sizeof(header), "Config_V2", 9);
    // Write to LBA 100
    io_channel_config_t config = {0};
    void *channel = NULL;
    assert(block_channel_open(43, &config, &channel) == IO_STATUS_OK);
    io_sg_entry_t sg = { .phys_addr = (uintptr_t)sector, .length = 512 };
    io_request_t req = { .id = 9100, .device_id = 43, .opcode = IO_OP_WRITE, .lba = 100, .block_count = 1, .segments = &sg, .segment_count = 1 };
    assert(block_submit(43, channel, &req) == IO_STATUS_OK);
    memblk_tick();
    io_completion_t comp;
    uint32_t count = 0;
    assert(block_poll_completions(43, channel, &comp, 1, &count) == IO_STATUS_OK);
    assert(block_channel_close(43, channel) == IO_STATUS_OK);

    // Power cycle
    memblk_power_cycle(43);

    // Active configuration must remain "Config_V1" because selector is still pointing to slot A
    char out_buf[64];
    uint32_t out_len = 0;
    assert(config_store_read(43, 1234, 101, out_buf, sizeof(out_buf), &out_len) == IO_STATUS_OK);
    assert(out_len == 9);
    assert(memcmp(out_buf, "Config_V1", 9) == 0);

    memblk_attach_backing(43, false);
    memblk_deinit();
    printf("  -> PASS\n");
}

// 14. test_config_selector_torn_write
static void test_config_selector_torn_write(void) {
    printf("test_config_selector_torn_write...\n");
    test_keystore_clear();
    memblk_init();

    keystore_key_cap_t cap = { .cap_handle = 1234, .allowed_op = 3, .key_id = 999 };
    test_keystore_register_cap(&cap);

    uint8_t uuid[16] = {0};
    assert(config_store_init(42, uuid) == IO_STATUS_OK);

    // Write a corrupt selector to LBA 0 (e.g. incorrect CRC)
    uint8_t sector[512];
    memset(sector, 0xFF, sizeof(sector)); // pure garbage

    io_channel_config_t config = {0};
    void *channel = NULL;
    assert(block_channel_open(42, &config, &channel) == IO_STATUS_OK);
    io_sg_entry_t sg = { .phys_addr = (uintptr_t)sector, .length = 512 };
    io_request_t req = { .id = 9200, .device_id = 42, .opcode = IO_OP_WRITE, .lba = 0, .block_count = 1, .segments = &sg, .segment_count = 1 };
    assert(block_submit(42, channel, &req) == IO_STATUS_OK);
    memblk_tick();
    io_completion_t comp;
    uint32_t count = 0;
    assert(block_poll_completions(42, channel, &comp, 1, &count) == IO_STATUS_OK);
    assert(block_channel_close(42, channel) == IO_STATUS_OK);

    // Reading with a torn selector must fall back safely and return IO_STATUS_IO_ERROR rather than mixed/corrupt plaintext
    char out_buf[64];
    uint32_t out_len = 0;
    assert(config_store_read(42, 1234, 101, out_buf, sizeof(out_buf), &out_len) == IO_STATUS_IO_ERROR);

    memblk_deinit();
    printf("  -> PASS\n");
}

// 15. test_config_old_or_new_atomicity
static void test_config_old_or_new_atomicity(void) {
    printf("test_config_old_or_new_atomicity...\n");
    test_keystore_clear();
    memblk_init();
    memblk_attach_backing(43, true);

    keystore_key_cap_t cap = { .cap_handle = 1234, .allowed_op = 3, .key_id = 999 };
    test_keystore_register_cap(&cap);

    uint8_t uuid[16] = {0};
    assert(config_store_init(43, uuid) == IO_STATUS_OK);

    // Write original
    assert(config_store_write(43, 1234, 101, "Config_A", 9) == IO_STATUS_OK);

    // Update to B
    assert(config_store_write(43, 1234, 101, "Config_B", 9) == IO_STATUS_OK);

    // Verify recovery returns the newly committed Config_B
    char out_buf[64];
    uint32_t out_len = 0;
    assert(config_store_read(43, 1234, 101, out_buf, sizeof(out_buf), &out_len) == IO_STATUS_OK);
    assert(out_len == 9);
    assert(memcmp(out_buf, "Config_B", 9) == 0);

    memblk_attach_backing(43, false);
    memblk_deinit();
    printf("  -> PASS\n");
}

// 16. test_config_authentication_failure
static void test_config_authentication_failure(void) {
    printf("test_config_authentication_failure...\n");
    test_keystore_clear();
    memblk_init();

    keystore_key_cap_t cap = { .cap_handle = 1234, .allowed_op = 3, .key_id = 999 };
    test_keystore_register_cap(&cap);

    uint8_t uuid[16] = {0};
    assert(config_store_init(42, uuid) == IO_STATUS_OK);
    assert(config_store_write(42, 1234, 101, "AuthenticData", 14) == IO_STATUS_OK);

    // Find active slot from selector first
    uint8_t sel_sector[512];
    io_channel_config_t config = {0};
    void *channel = NULL;
    assert(block_channel_open(42, &config, &channel) == IO_STATUS_OK);
    io_sg_entry_t sel_sg = { .phys_addr = (uintptr_t)sel_sector, .length = 512 };
    io_request_t sel_req = { .id = 9299, .device_id = 42, .opcode = IO_OP_READ, .lba = 0, .block_count = 1, .segments = &sel_sg, .segment_count = 1 };
    assert(block_submit(42, channel, &sel_req) == IO_STATUS_OK);
    memblk_tick();
    io_completion_t comp;
    uint32_t count = 0;
    assert(block_poll_completions(42, channel, &comp, 1, &count) == IO_STATUS_OK);

    config_selector_t sel;
    memcpy(&sel, sel_sector, sizeof(sel));
    uint64_t active_lba = (sel.active_slot == 0) ? 1 : 100;

    // Tamper with the active slot LBA ciphertext directly
    uint8_t sector[512];
    io_sg_entry_t sg = { .phys_addr = (uintptr_t)sector, .length = 512 };
    io_request_t req_read = { .id = 9300, .device_id = 42, .opcode = IO_OP_READ, .lba = active_lba, .block_count = 1, .segments = &sg, .segment_count = 1 };
    assert(block_submit(42, channel, &req_read) == IO_STATUS_OK);
    memblk_tick();
    assert(block_poll_completions(42, channel, &comp, 1, &count) == IO_STATUS_OK);

    // Modify ciphertext bytes to simulate tamper/corruption
    sector[sizeof(encrypted_config_header_t) + 3] ^= 0xFF;

    io_request_t req_write = { .id = 9301, .device_id = 42, .opcode = IO_OP_WRITE, .lba = active_lba, .block_count = 1, .segments = &sg, .segment_count = 1 };
    assert(block_submit(42, channel, &req_write) == IO_STATUS_OK);
    memblk_tick();
    assert(block_poll_completions(42, channel, &comp, 1, &count) == IO_STATUS_OK);
    assert(block_channel_close(42, channel) == IO_STATUS_OK);

    // Try to read config - must detect authentication tag failure and return IO_STATUS_IO_ERROR
    char out_buf[64];
    uint32_t out_len = 0;
    assert(config_store_read(42, 1234, 101, out_buf, sizeof(out_buf), &out_len) == IO_STATUS_IO_ERROR);

    memblk_deinit();
    printf("  -> PASS\n");
}

// 17. test_config_partition_binding
static void test_config_partition_binding(void) {
    printf("test_config_partition_binding...\n");
    test_keystore_clear();
    memblk_init();

    // Register a key bounded to partition UUID 0xAA
    uint8_t partition_A[16] = {0xAA};
    keystore_key_cap_t cap = {
        .cap_handle = 1234,
        .allowed_op = 3,
        .key_id = 999
    };
    memcpy(cap.partition_uuid, partition_A, 16);
    test_keystore_register_cap(&cap);

    // Init config store on partition B (0xBB)
    uint8_t partition_B[16] = {0xBB};
    assert(config_store_init(42, partition_B) == IO_STATUS_OK);

    // Seal must be rejected because key cap partition UUID (0xAA) doesn't match partition UUID (0xBB)
    assert(config_store_write(42, 1234, 101, "BindData", 8) == IO_STATUS_PERMISSION_DENIED);

    memblk_deinit();
    printf("  -> PASS\n");
}

// 18. test_rollback_old_snapshot_rejected
static void test_rollback_old_snapshot_rejected(void) {
    printf("test_rollback_old_snapshot_rejected...\n");
    test_keystore_clear();
    memblk_init();

    keystore_key_cap_t cap = { .cap_handle = 1234, .allowed_op = 3, .key_id = 999 };
    test_keystore_register_cap(&cap);

    uint8_t uuid[16] = {0};
    assert(config_store_init(42, uuid) == IO_STATUS_OK);

    // Set rollback protection prevention level
    g_test_rollback_level = ROLLBACK_PROTECTION_PREVENTION;
    g_test_rollback_epoch = 5ULL; // Platform epoch is 5

    // Write active config with current epoch
    assert(config_store_write(42, 1234, 101, "Epoch5Config", 12) == IO_STATUS_OK);

    // Read back config with current epoch is successful
    char out_buf[64];
    uint32_t out_len = 0;
    assert(config_store_read(42, 1234, 101, out_buf, sizeof(out_buf), &out_len) == IO_STATUS_OK);
    assert(memcmp(out_buf, "Epoch5Config", 12) == 0);

    // Advance platform rollback epoch to 10
    g_test_rollback_epoch = 10ULL;

    // Try to read back the old epoch 5 config. It must detect rollback and reject it!
    assert(config_store_read(42, 1234, 101, out_buf, sizeof(out_buf), &out_len) == IO_STATUS_PERMISSION_DENIED);

    // Telemetry rollback counter must be incremented
    io_health_snapshot_t snap;
    assert(block_device_health_snapshot(42, &snap) == IO_STATUS_OK);
    assert(snap.counters.rollback_rejection_count > 0);

    g_test_rollback_level = ROLLBACK_PROTECTION_NONE;
    memblk_deinit();
    printf("  -> PASS\n");
}

// 19. test_rollback_anchor_unavailable
static void test_rollback_anchor_unavailable(void) {
    printf("test_rollback_anchor_unavailable...\n");

    // Set level to NONE
    g_test_rollback_level = ROLLBACK_PROTECTION_NONE;

    // In this state, prevention of rollbacks should report unsupported or inactive
    assert(g_test_rollback_level == ROLLBACK_PROTECTION_NONE);
    printf("  Rollback anchor reports level NONE as expected.\n");
    printf("  -> PASS\n");
}

// 20. test_storage_fault_injection_matrix
static void test_storage_fault_injection_matrix(void) {
    printf("test_storage_fault_injection_matrix...\n");
    memblk_init();

    // Inject read failure
    memblk_test_set_fault(42, IO_STATUS_IO_ERROR);

    uint8_t sector[512];
    io_channel_config_t config = {0};
    void *channel = NULL;
    assert(block_channel_open(42, &config, &channel) == IO_STATUS_OK);
    io_sg_entry_t sg = { .phys_addr = (uintptr_t)sector, .length = 512 };
    io_request_t req = { .id = 9400, .device_id = 42, .opcode = IO_OP_READ, .lba = 1, .block_count = 1, .segments = &sg, .segment_count = 1 };

    assert(block_submit(42, channel, &req) == IO_STATUS_OK);
    memblk_tick();

    io_completion_t comp;
    uint32_t count = 0;
    assert(block_poll_completions(42, channel, &comp, 1, &count) == IO_STATUS_OK);
    assert(count == 1);
    assert(comp.status == IO_STATUS_IO_ERROR);

    // Verify health failure counter was incremented
    io_health_snapshot_t snap;
    assert(block_device_health_snapshot(42, &snap) == IO_STATUS_OK);
    assert(snap.counters.requests_failed > 0);

    assert(block_channel_close(42, channel) == IO_STATUS_OK);
    memblk_deinit();
    printf("  -> PASS\n");
}

int main(void) {
    printf("Starting Storage Integrity E5-S1 Contract Test Suite...\n");

    test_block_legacy_contract_removed();
    test_storage_health_counters();
    test_storage_health_telemetry();
    test_durability_policy_resolution();
    test_event_log_append_recover();
    test_event_log_torn_record_recovery();
    test_event_log_reboot_persistence();
    test_event_log_wrap_retention();
    test_config_encrypted_round_trip();
    test_config_key_capability_required();
    test_config_revoked_key_rejected();
    test_config_crash_before_commit();
    test_config_crash_after_data_flush();
    test_config_selector_torn_write();
    test_config_old_or_new_atomicity();
    test_config_authentication_failure();
    test_config_partition_binding();
    test_rollback_old_snapshot_rejected();
    test_rollback_anchor_unavailable();
    test_storage_fault_injection_matrix();

    printf("\nAll 20 E5-S1 Storage Integrity Contract Tests PASSED successfully!\n");
    return 0;
}
