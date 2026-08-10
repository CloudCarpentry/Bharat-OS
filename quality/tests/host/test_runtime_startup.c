#include <assert.h>
#include <stddef.h>
#include <stdint.h>

#include <bharat/runtime/runtime.h>
#include <bharat/uapi/init/bootstrap.h>
#include <bharat/uapi/syscall/bh_syscall.h>

extern const bharat_user_startup_t *bharat_runtime_get_startup(void);

int64_t bharat_syscall(int64_t number, int64_t arg0, int64_t arg1,
                       int64_t arg2, int64_t arg3, int64_t arg4,
                       int64_t arg5) {
    (void)number;
    (void)arg0;
    (void)arg1;
    (void)arg2;
    (void)arg3;
    (void)arg4;
    (void)arg5;
    return 0;
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
    return 0;
}
