#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "../subsys/include/bharat/automotive/automotive.h"

static int g_started = 0;

static void boot_start_hook(uint32_t service_id) {
    (void)service_id;
    g_started++;
}

static bool eth_hook(uint16_t ethertype, const uint8_t* payload, uint16_t payload_len, uint8_t traffic_class) {
    return (ethertype == 0x88F7U && payload && payload_len > 0U && traffic_class <= 7U);
}

int main(void) {
    subsys_automotive_init();

    uint8_t payload[8] = {0};
    assert(subsys_automotive_send_lin_frame(0x1AU, payload, 8U));
    assert(!subsys_automotive_send_lin_frame(0x7FU, payload, 8U));

    uint64_t mono = 0U;
    uint64_t bus = 0U;
    assert(!subsys_automotive_get_last_time_sync(&mono, &bus));
    subsys_automotive_emit_time_sync(1000U, 900U);
    assert(subsys_automotive_get_last_time_sync(&mono, &bus));
    assert(mono == 1000U);
    assert(bus == 900U);

    assert(!subsys_automotive_send_ethernet_frame(0x88F7U, payload, 8U, 3U));
    subsys_automotive_register_ethernet_hook(eth_hook);
    assert(subsys_automotive_send_ethernet_frame(0x88F7U, payload, 8U, 3U));

    automotive_boot_service_t s0 = {.service_id = 1U, .dependency_count = 0U, .essential_for_minimal_lane = 1U};
    automotive_boot_service_t s1 = {.service_id = 2U, .dependency_count = 1U, .essential_for_minimal_lane = 0U};
    assert(subsys_automotive_register_boot_service(&s0));
    assert(subsys_automotive_register_boot_service(&s1));

    uint32_t ids[4] = {0};
    g_started = 0;
    assert(subsys_automotive_run_boot_stage(0U, boot_start_hook, ids, 4U) == 1U);
    assert(g_started == 1);

    subsys_automotive_select_boot_profile(AUTOMOTIVE_BOOT_PROFILE_RT_MINIMAL);
    assert(subsys_automotive_run_boot_stage(1U, boot_start_hook, ids, 4U) == 0U);

    // Queue registration tests
    assert(!subsys_automotive_queue_register(1U, NULL));

    automotive_bounded_queue_t q_cfg_zero_cap = {.capacity = 0U};
    assert(!subsys_automotive_queue_register(2U, &q_cfg_zero_cap));

    automotive_bounded_queue_t q_cfg_over_max = {.capacity = BHARAT_AUTOMOTIVE_MAX_QUEUE_DEPTH + 1U};
    assert(!subsys_automotive_queue_register(3U, &q_cfg_over_max));

    automotive_bounded_queue_t q_cfg_valid = {.capacity = BHARAT_AUTOMOTIVE_MAX_QUEUE_DEPTH};
    assert(subsys_automotive_queue_register(4U, &q_cfg_valid));

    printf("Automotive subsystem tests passed.\n");
    return 0;
}
