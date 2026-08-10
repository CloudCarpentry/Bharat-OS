#include <bharat/uapi/bootstrap/root_launch.h>
#include <bharat/uapi/init/bootstrap.h>
#include <bharat/uapi/syscall/bh_syscall.h>
#include <bharat/uapi/syscall_nr.h>
#include <stddef.h>

extern const bharat_user_startup_t *bharat_runtime_get_startup(void);

static size_t light_strlen(const char *text) {
    size_t length = 0;
    while (text[length] != '\0') {
        ++length;
    }
    return length;
}

static void light_log(const char *text) {
    (void)bharat_syscall(SYSCALL_WRITE, 1, (uintptr_t)text,
                         light_strlen(text), 0, 0, 0);
}

int main(void) {
    const bharat_user_startup_t *startup = bharat_runtime_get_startup();
    light_log("INIT_LITE: ENTERED\n");
    if (startup == NULL ||
        (startup->flags & BH_USER_STARTUP_FLAG_ROOT_LAUNCH_EXTENSION) == 0) {
        light_log("INIT_LITE: STARTUP_ABI_INVALID\n");
        return 1;
    }
    const bh_root_launch_info_t *launch =
        (const bh_root_launch_info_t *)((const uint8_t *)startup +
                                       startup->struct_size);
    if (launch->version != BH_ROOT_LAUNCH_ABI_VERSION ||
        launch->size != sizeof(*launch) ||
        launch->runtime_model != BH_USERSPACE_RUNTIME_LIGHT) {
        light_log("INIT_LITE: ROOT_LAUNCH_INVALID\n");
        return 2;
    }
    light_log("LIGHT_RUNTIME: STABLE\n");
    for (;;) {
        (void)bharat_syscall(SYSCALL_SCHED_YIELD, 0, 0, 0, 0, 0, 0);
    }
}
