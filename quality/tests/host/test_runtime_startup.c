#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <bharat/runtime/runtime.h>
#include <bharat/uapi/init/bootstrap.h>
#include <bharat/uapi/syscall/bh_syscall.h>
#include <bharat/uapi/syscall_nr.h>
#include <bharat/syscalls.h>

extern const bharat_user_startup_t *bharat_runtime_get_startup(void);
extern void bharat_runtime_log(const char *msg);
extern int bharat_runtime_main_wrapper(int argc, char **argv, int (*main_fn)(int, char**));

static char g_mock_write_buf[8192];
static size_t g_mock_write_len = 0;
static size_t g_mock_write_called = 0;
static int64_t g_mock_write_return_override = -999;

int64_t bharat_syscall(int64_t number, int64_t arg0, int64_t arg1,
                       int64_t arg2, int64_t arg3, int64_t arg4,
                       int64_t arg5) {
    if (number == SYSCALL_WRITE) {
        g_mock_write_called++;

        const char *buf = (const char *)(uintptr_t)arg1;
        size_t len = (size_t)arg2;

        // Let's use a magic value to indicate "no override" instead of 0
        if (g_mock_write_return_override != -999) {
            int64_t ret = g_mock_write_return_override;
            if (ret > 0) {
                size_t c = (size_t)ret > len ? len : (size_t)ret;
                for (size_t i = 0; i < c; i++) {
                    if (g_mock_write_len < sizeof(g_mock_write_buf)) {
                        g_mock_write_buf[g_mock_write_len++] = buf[i];
                    }
                }
            }
            return ret;
        }

        for (size_t i = 0; i < len; i++) {
            if (g_mock_write_len < sizeof(g_mock_write_buf)) {
                g_mock_write_buf[g_mock_write_len++] = buf[i];
            }
        }
        return len;
    }

    (void)number;
    (void)arg0;
    (void)arg1;
    (void)arg2;
    (void)arg3;
    (void)arg4;
    (void)arg5;
    return 0;
}

int bharat_sched_yield(void) {
    return 0;
}

static int mock_main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    return 42;
}

static void reset_mock(void) {
    g_mock_write_called = 0;
    g_mock_write_len = 0;
    g_mock_write_return_override = -999;
    memset(g_mock_write_buf, 0, sizeof(g_mock_write_buf));
}

void test_runtime_log(void) {
    // Null message
    reset_mock();
    bharat_runtime_log(NULL);
    assert(g_mock_write_called == 0);

    // Empty message
    reset_mock();
    bharat_runtime_log("");
    assert(g_mock_write_called == 0);

    // Ordinary message
    reset_mock();
    bharat_runtime_log("Hello, World!");
    assert(g_mock_write_called == 1);
    assert(g_mock_write_len == 13);
    assert(strncmp(g_mock_write_buf, "Hello, World!", 13) == 0);

    // Partial write sequence (3 bytes per call)
    reset_mock();
    g_mock_write_return_override = 3;
    bharat_runtime_log("Partial Write Test");
    assert(g_mock_write_called == 6); // 18 / 3
    assert(g_mock_write_len == 18);
    assert(strncmp(g_mock_write_buf, "Partial Write Test", 18) == 0);

    // Zero-byte write handling
    reset_mock();
    g_mock_write_return_override = 0;
    bharat_runtime_log("Should break early");
    assert(g_mock_write_called == 1); // Breaks immediately
    assert(g_mock_write_len == 0);

    // Negative syscall error
    reset_mock();
    g_mock_write_return_override = -5;
    bharat_runtime_log("Should break early");
    assert(g_mock_write_called == 1);
    assert(g_mock_write_len == 0);

    // Invalid positive return length (greater than remaining)
    reset_mock();
    g_mock_write_return_override = 100;
    bharat_runtime_log("Short");
    assert(g_mock_write_called == 1);
    assert(g_mock_write_len == 5); // Written truncated to remaining len

    // 4095, 4096 and over-limit message bounds
    char long_msg[4100];
    for (int i = 0; i < 4099; i++) long_msg[i] = 'A';
    long_msg[4099] = '\0';

    // 4095
    long_msg[4095] = '\0';
    reset_mock();
    bharat_runtime_log(long_msg);
    assert(g_mock_write_len == 4095);

    // 4096
    long_msg[4095] = 'A';
    long_msg[4096] = '\0';
    reset_mock();
    bharat_runtime_log(long_msg);
    assert(g_mock_write_len == 4096);

    // 4099 (should truncate at 4096)
    long_msg[4096] = 'A';
    long_msg[4099] = '\0';
    reset_mock();
    bharat_runtime_log(long_msg);
    assert(g_mock_write_len == 4096);
}

void test_main_wrapper(void) {
    bharat_user_startup_t startup = {0};
    startup.bootstrap.bootstrap_cap = 99;

    bharat_runtime_init(&startup);
    assert(bharat_runtime_get_startup() == &startup);

    char *argv[] = {"test", NULL};
    int res = bharat_runtime_main_wrapper(1, argv, mock_main);
    assert(res == 42);

    // Make sure startup context wasn't reset to NULL
    assert(bharat_runtime_get_startup() == &startup);
    assert(bharat_runtime_get_bootstrap_cap() == 99);
}

int main(void) {
    bharat_user_startup_t startup = {0};
    startup.bootstrap.bootstrap_cap = 42;

    bharat_runtime_init((const void *)(uintptr_t)19U);
    assert(bharat_runtime_get_startup() == NULL);
    assert(bharat_runtime_get_bootstrap_cap() == BHARAT_INVALID_HANDLE);

    bharat_runtime_init(&startup);
    assert(bharat_runtime_get_startup() == &startup);
    assert(bharat_runtime_get_bootstrap_cap() == 42);

    bharat_runtime_init((const void *)(uintptr_t)19U);
    assert(bharat_runtime_get_startup() == NULL);
    assert(bharat_runtime_get_bootstrap_cap() == BHARAT_INVALID_HANDLE);

    test_runtime_log();
    test_main_wrapper();
    return 0;
}
