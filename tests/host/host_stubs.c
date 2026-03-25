#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "../../kernel/include/hal/hal.h"
#include "../../kernel/include/core/multikernel.h"

// Unified host mocks
__attribute__((weak)) uint32_t hal_cpu_get_id(void) {
    return 0; // Default to core 0 for host tests unless overridden
}

__attribute__((weak)) void hal_cpu_disable_interrupts(void) {}
__attribute__((weak)) void hal_cpu_enable_interrupts(void) {}

__attribute__((weak)) void hal_send_ipi_payload(uint32_t target_core, uint64_t payload) {
    (void)target_core;
    (void)payload;
}

__attribute__((weak)) void hal_ipi_send(uint32_t target_core, uint32_t vector) {
    (void)target_core;
    (void)vector;
}

__attribute__((weak)) void kernel_panic(const char *msg) {
    printf("KERNEL PANIC (host test stub): %s\n", msg);
    exit(1);
}

__attribute__((weak)) int mk_get_channel(uint32_t sender_core, uint32_t receiver_core, mk_channel_t* out_channel) {
    (void)sender_core;
    (void)receiver_core;
    if (out_channel) {
        *out_channel = (mk_channel_t){0};
    }
    return 0;
}

__attribute__((weak)) int mk_send_message(mk_channel_t *channel, uint32_t msg_type, void *payload, uint32_t payload_size) {
    (void)channel;
    (void)msg_type;
    (void)payload;
    (void)payload_size;
    return 0;
}
