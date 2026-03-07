#include <stdio.h>
#include <assert.h>
#include <setjmp.h>
#include <string.h>
#include <stdint.h>

#include "kernel.h"
#include "hal/hal.h"

static int g_panic_triggered = 0;
static int g_dump_state_called = 0;
static jmp_buf g_panic_jmp_env;

// Mocking the pstore linker symbols with arrays to allocate space
uint8_t _pstore_data[4096];
uint8_t _pstore_data_end;
extern uint8_t _pstore_start __attribute__((alias("_pstore_data")));
extern uint8_t _pstore_end __attribute__((alias("_pstore_data_end")));

// Mocks
void hal_cpu_disable_interrupts(void) {}
void hal_serial_write(const char *s) { (void)s; }

void hal_cpu_dump_state(void) {
    g_dump_state_called = 1;
}

void hal_cpu_reboot(void) {
    g_panic_triggered = 1;
    longjmp(g_panic_jmp_env, 1);
}

void hal_cpu_halt(void) {
    g_panic_triggered = 1;
    longjmp(g_panic_jmp_env, 1);
}

int main(void) {
    printf("[TEST] Running Panic Handler Tests...\n");

    // We will test the panic handler. To prevent the infinite loop from actually halting,
    // we use setjmp/longjmp.

    if (setjmp(g_panic_jmp_env) == 0) {
        // Trigger panic
        kernel_panic("test panic message");
        // Should not reach here
        assert(0 && "kernel_panic returned!");
    } else {
        // Panic halted/rebooted successfully
        assert(g_panic_triggered == 1);
        assert(g_dump_state_called == 1);
        printf("[TEST] Panic correctly halted/rebooted execution and dumped state.\n");
    }

    printf("[TEST] Panic Handler Tests Passed.\n");
    return 0;
}
