#include <stdio.h>
#include <assert.h>
// Inclusion test to ensure no redefinition conflicts
#include <stddef.h>
#include <stdint.h>
#include "../lib/posix/include/unistd.h"

#include "../subsys/linux/linux_compat.h"

int main() {
    printf("Running POSIX header integration tests...\n");

    // Check size of newly added types to ensure they are available
    assert(sizeof(uid_t) == 4);
    assert(sizeof(gid_t) == 4);
    assert(sizeof(pid_t) == 4);

    // Test Linux compatibility layer
    subsys_instance_t linux_env = {
        .type = SUBSYS_TYPE_LINUX,
        .is_running = 1
    };

    assert(linux_subsys_init(&linux_env) == 0);

    // Simulate "echo hello"
    // write(1, "hello\n", 6)
    long ret = linux_syscall_handler(1, 1, (long)"hello\n", 6, 0, 0, 0);
    assert(ret == 6);

    // Simulate "ls" (open current dir, read, close)
    // open(".", O_RDONLY) -> fd 3
    long fd = linux_syscall_handler(2, (long)".", 0, 0, 0, 0, 0);
    assert(fd >= 3);

    // read(fd, buf, size)
    char buf[128];
    ret = linux_syscall_handler(0, fd, (long)buf, sizeof(buf), 0, 0, 0);
    assert(ret == 0); // Stub returns 0

    // close(fd)
    ret = linux_syscall_handler(3, fd, 0, 0, 0, 0, 0);
    assert(ret == 0);

    // Simulate exit(0)
    ret = linux_syscall_handler(60, 0, 0, 0, 0, 0, 0);
    assert(ret == 0);
    assert(linux_env.is_running == 0);

    printf("POSIX header integration tests passed successfully.\n");
    return 0;
}
