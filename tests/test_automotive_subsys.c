#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "../drivers/include/bharat/drivers/can.h"
#include "../subsys/include/bharat/automotive/automotive.h"

static int g_started = 0;
static int g_power_mode_events = 0;

static void boot_start_hook(uint32_t service_id) {
    (void)service_id;
    g_started++;
}

static bool eth_hook(uint16_t ethertype, const uint8_t* payload, uint16_t payload_len, uint8_t traffic_class) {
    return (ethertype == 0x88F7U && payload && payload_len > 0U && traffic_class <= 7U);
}

static void power_mode_hook(automotive_power_mode_t from_mode, automotive_power_mode_t to_mode) {
    if (from_mode != to_mode) {
        g_power_mode_events++;
    }
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

    // Queue push tests
    assert(!subsys_automotive_queue_push(999U, 1U)); // Unregistered queue

    automotive_bounded_queue_t q_cfg_push = {
        .capacity = 2U,
        .priority_aware_wakeup = 1U,
        .high_watermark = 0U
    };
    assert(subsys_automotive_queue_register(5U, &q_cfg_push));

    assert(subsys_automotive_queue_push(5U, 10U)); // Success, depth becomes 1, watermark 10
    assert(subsys_automotive_queue_push(5U, 5U));  // Success, depth becomes 2, watermark still 10 (5 < 10)
    assert(!subsys_automotive_queue_push(5U, 20U)); // Fails, depth is 2 (at capacity)

    // VNS network controller tests
    automotive_net_controller_t can_ctrl = {
        .controller_id = 42U,
        .bus = AUTOMOTIVE_BUS_CAN_CLASSIC,
        .max_dlc = 8U,
        .online = 1U
    };
    assert(subsys_automotive_register_net_controller(&can_ctrl));
    assert(subsys_automotive_send_vns_frame(42U, 0x101U, payload, 8U, 2U));
    assert(subsys_automotive_get_vns_tx_count(42U) == 1U);
    assert(subsys_automotive_set_net_controller_online(42U, false));
    assert(!subsys_automotive_send_vns_frame(42U, 0x101U, payload, 8U, 2U));

    // Safety emergency path tests
    assert(!subsys_automotive_is_emergency_stop_active());
    subsys_automotive_trigger_emergency_stop(0xDEADU);
    assert(subsys_automotive_is_emergency_stop_active());
    assert(subsys_automotive_get_emergency_reason() == 0xDEADU);
    assert(!subsys_automotive_send_vns_frame(42U, 0x101U, payload, 8U, 2U));
    subsys_automotive_clear_emergency_stop();
    assert(!subsys_automotive_is_emergency_stop_active());

    // Power mode manager tests
    subsys_automotive_register_power_mode_hook(power_mode_hook);
    assert(subsys_automotive_get_power_mode() == AUTOMOTIVE_POWER_MODE_OFF);
    assert(subsys_automotive_request_power_mode(AUTOMOTIVE_POWER_MODE_ACCESSORY));
    assert(subsys_automotive_request_power_mode(AUTOMOTIVE_POWER_MODE_DRIVE));
    assert(!subsys_automotive_request_power_mode(AUTOMOTIVE_POWER_MODE_ACCESSORY)); // invalid from DRIVE
    assert(g_power_mode_events >= 2);

    // CAN loopback driver tests
    can_loopback_init();
    can_loopback_reset();
    assert(can_loopback_pending() == 0U);
    assert(can_loopback_send(0x120U, payload, 8U));
    assert(can_loopback_pending() == 1U);

    uint32_t rx_id = 0U;
    uint8_t rx_dlc = 0U;
    uint8_t rx_data[64] = {0};
    assert(can_loopback_receive(&rx_id, rx_data, &rx_dlc));
    assert(rx_id == 0x120U);
    assert(rx_dlc == 8U);
    assert(can_loopback_pending() == 0U);

    printf("Automotive subsystem tests passed.\n");
    return 0;
}
