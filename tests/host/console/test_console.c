#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>

#include "console/console_format.h"
#include "console/console_base_types.h"
#include "console/console_record.h"
#include "console/console_caps.h"
#include "console/console_buffer.h"
#include "console/console_backend.h"
#include "console/console_router.h"
#include "console/console_policy.h"
#include "console/console_core.h"
#include "console/console_discovery.h"

// Define global state required by router and core
extern console_global_state_t g_console_state;

static int format_wrapper(char *buf, size_t size, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int len = console_vsnprintf(buf, size, fmt, ap);
    va_end(ap);
    return len;
}

int test_formatter(void) {
    char buf[128];
    int len;

    printf("Running formatter tests...\n");

    // Test %s
    len = format_wrapper(buf, sizeof(buf), "Hello %s", "World");
    if (strcmp(buf, "Hello World") != 0 || len != 11) return -1;

    // Test %c
    len = format_wrapper(buf, sizeof(buf), "Char %c", 'A');
    if (strcmp(buf, "Char A") != 0 || len != 6) return -1;

    // Test %d
    len = format_wrapper(buf, sizeof(buf), "Num %d", -123);
    if (strcmp(buf, "Num -123") != 0 || len != 8) return -1;

    // Test %u
    len = format_wrapper(buf, sizeof(buf), "Unum %u", 456);
    if (strcmp(buf, "Unum 456") != 0 || len != 8) return -1;

    // Test %x
    len = format_wrapper(buf, sizeof(buf), "Hex %x", 0x1A2B);
    if (strcmp(buf, "Hex 1a2b") != 0 || len != 8) return -1;

    // Test bounds
    char small_buf[5];
    len = format_wrapper(small_buf, sizeof(small_buf), "123456789");
    if (strcmp(small_buf, "1234") != 0 || len != 9) return -1;

    return 0;
}

int test_types_and_layout(void) {
    printf("Running type layout tests...\n");

    if (sizeof(console_caps_t) != 4) return -1;
    if (sizeof(console_seq_t) != 4) return -1;
    if (sizeof(console_time_t) != 8) return -1;
    if (sizeof(console_flags_t) != 2) return -1;
    if (sizeof(console_len_t) != 2) return -1;

    // Ensure `console_record_t` is relatively compact
    if (sizeof(console_record_t) > 256) return -1;

    return 0;
}

int test_buffer_ring(void) {
    printf("Running buffer ring tests...\n");

    console_buffer_init();

    // Push more than capacity
    for (int i = 0; i < CONSOLE_RING_CAPACITY + 10; i++) {
        console_record_t rec = {0};
        rec.level = CONSOLE_LEVEL_INFO;
        format_wrapper(rec.text, sizeof(rec.text), "Log %d", i);
        rec.text_len = strlen(rec.text);

        console_buffer_push(&rec);
    }

    if (console_buffer_dropped_count() != 10) return -1;

    console_record_t oldest;
    if (!console_buffer_peek_oldest(&oldest, 0)) return -1;

    // Oldest should be Log 10 because 0-9 were overwritten
    if (strcmp(oldest.text, "Log 10") != 0) return -1;

    return 0;
}

static size_t mock_write_calls = 0;
static size_t mock_write(console_backend_t *backend, const char *data, size_t len) {
    (void)backend;
    (void)data;
    (void)len;
    mock_write_calls++;
    return len;
}

int test_routing(void) {
    printf("Running routing tests...\n");

    console_early_init();

    console_backend_ops_t mock_ops = { .write = mock_write };
    console_backend_t mock_backend = {
        .name = "mock",
        .enabled = true,
        .ops = &mock_ops,
        .early_ok = true,
        .min_level = CONSOLE_LEVEL_INFO,
        .caps = CON_CAP_EARLY_BOOT | CON_CAP_RUNTIME
    };

    console_register_backend(&mock_backend);

    mock_write_calls = 0;

    // Test normal log
    console_log(CONSOLE_LEVEL_INFO, "Info test");
    if (mock_write_calls != 1) return -1;

    // Test filtered log (DEBUG is below INFO)
    console_log(CONSOLE_LEVEL_DEBUG, "Debug test");
    if (mock_write_calls != 1) return -1; // Should not increment

    // Test panic routing
    mock_backend.panic_ok = false;
    console_enter_panic(); // Switch phase
    console_log(CONSOLE_LEVEL_INFO, "Panic test");
    if (mock_write_calls != 1) return -1; // Should be filtered because !panic_ok

    return 0;
}

int test_policy(void) {
    printf("Running policy tests...\n");

    console_device_desc_t devs[3] = {0};

    // Serial
    devs[0].type = CONSOLE_BACKEND_SERIAL;
    devs[0].early_ok = 1;
    devs[0].panic_ok = 1;
    devs[0].priority = 50;
    devs[0].caps = CON_CAP_WRITE_POLL;

    // Framebuffer
    devs[1].type = CONSOLE_BACKEND_FRAMEBUFFER;
    devs[1].early_ok = 0;
    devs[1].panic_ok = 0;
    devs[1].priority = 100;
    devs[1].caps = CON_CAP_VISIBLE_SINK;

    // Debug Port
    devs[2].type = CONSOLE_BACKEND_DEBUGPORT;
    devs[2].early_ok = 1;
    devs[2].panic_ok = 1;
    devs[2].priority = 10;
    devs[2].caps = CON_CAP_WRITE_POLL;

    console_policy_decision_t decision;
    console_choose_policy(devs, 3, &decision);

    // Serial (index 0) has higher priority than Debug Port (index 2) for early
    if (decision.early_primary_index != 0) return -1;

    // Framebuffer (index 1) is visible sink, highest priority for runtime
    if (decision.runtime_primary_index != 1) return -1;

    // Serial (index 0) is polling-safe, highest priority for panic
    if (decision.panic_primary_index != 0) return -1;

    return 0;
}

int main(void) {
    if (test_formatter() != 0) {
        printf("Formatter test failed!\n");
        return 1;
    }

    if (test_types_and_layout() != 0) {
        printf("Type layout test failed!\n");
        return 1;
    }

    if (test_buffer_ring() != 0) {
        printf("Buffer ring test failed!\n");
        return 1;
    }

    if (test_routing() != 0) {
        printf("Routing test failed!\n");
        return 1;
    }

    if (test_policy() != 0) {
        printf("Policy test failed!\n");
        return 1;
    }

    printf("All console tests passed successfully!\n");
    return 0;
}
