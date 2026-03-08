#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "../kernel/include/advanced/multikernel.h"

int multicore_boot_secondary_cores(uint32_t core_count) {
    (void)core_count;
    return 0;
}

void sched_notify_ipc_ready(uint32_t core_id, uint32_t msg_type) {
    (void)core_id;
    (void)msg_type;
}

void hal_send_ipi_payload(uint32_t target_core, uint64_t payload) {
    (void)target_core;
    (void)payload;
}

int main(void) {
    urpc_msg_t buffer[2] = {0};
    urpc_ring_t ring = {0};
    urpc_msg_t msg = { .msg_type = 7U, .payload_size = 8U, .payload_data = {0xABCDU} };
    urpc_msg_t out = {0};

    assert(urpc_init_ring(NULL, buffer, 2U) == URPC_ERR_INVAL);
    assert(urpc_init_ring(&ring, NULL, 2U) == URPC_ERR_INVAL);
    assert(urpc_init_ring(&ring, buffer, 0U) == URPC_ERR_INVAL);
    assert(urpc_init_ring(&ring, buffer, 1U) == URPC_ERR_INVAL);
    assert(urpc_init_ring(&ring, buffer, 2U) == URPC_SUCCESS);

    assert(urpc_receive(&ring, &out) == URPC_ERR_EMPTY);
    assert(urpc_send(&ring, &msg) == URPC_SUCCESS);
    assert(urpc_send(&ring, &msg) == URPC_ERR_FULL);

    assert(urpc_receive(&ring, &out) == URPC_SUCCESS);
    assert(out.msg_type == 7U);
    assert(out.payload_size == 8U);
    assert(out.payload_data[0] == 0xABCDU);

    assert(urpc_receive(&ring, &out) == URPC_ERR_EMPTY);

    ring.capacity = 0U;
    assert(urpc_send(&ring, &msg) == URPC_ERR_INVAL);
    assert(urpc_receive(&ring, &out) == URPC_ERR_INVAL);

    printf("URPC ring tests passed.\n");
    return 0;
}
