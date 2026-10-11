#include <bharat/system.h>
#include <bharat/syscalls.h>
#include <stdlib.h>

void bh_runtime_abort(void) {
    bh_sys_thread_exit(-1);
    while (1) {}
}

void bh_runtime_debug_print(const char *msg) {
    if (!msg) return;
    int len = 0;
    while (msg[len]) len++;
    bh_sys_write(1, msg, len);
}
