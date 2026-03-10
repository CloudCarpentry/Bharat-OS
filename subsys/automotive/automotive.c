#include "bharat/automotive/automotive.h"

#define AUTOMOTIVE_INTERNAL_QUEUE_SLOTS 8U

static automotive_runtime_policy_t g_policy;
static automotive_deadline_hook_t g_deadline_hook;
static automotive_time_sync_hook_t g_time_sync_hook;
static automotive_ethernet_hook_t g_ethernet_hook;
static uint64_t g_last_monotonic_time_ns;
static uint64_t g_last_bus_time_ns;

typedef struct {
    uint8_t in_use;
    uint32_t queue_id;
    automotive_bounded_queue_t cfg;
} queue_slot_t;

static queue_slot_t g_queues[AUTOMOTIVE_INTERNAL_QUEUE_SLOTS];
static automotive_watchdog_t g_watchdogs[BHARAT_AUTOMOTIVE_MAX_WATCHDOGS];
static automotive_health_state_t g_domain_health[AUTOMOTIVE_FAULT_DOMAIN_GENERIC + 1U];
static automotive_boot_service_t g_boot_services[BHARAT_AUTOMOTIVE_MAX_BOOT_SERVICES];
static size_t g_boot_service_count;

static queue_slot_t* find_queue(uint32_t queue_id) {
    for (size_t i = 0; i < AUTOMOTIVE_INTERNAL_QUEUE_SLOTS; ++i) {
        if (g_queues[i].in_use && g_queues[i].queue_id == queue_id) {
            return &g_queues[i];
        }
    }
    return 0;
}

void subsys_automotive_init(void) {
    g_policy.ipc_mode = AUTOMOTIVE_IPC_MODE_DETERMINISTIC;
    g_policy.boot_profile = AUTOMOTIVE_BOOT_PROFILE_SAFE_MODE;
    g_policy.default_queue_budget_us = 250U;
    g_policy.default_deadline_slack_us = 50U;

    g_deadline_hook = 0;
    g_time_sync_hook = 0;
    g_ethernet_hook = 0;
    g_last_monotonic_time_ns = 0U;
    g_last_bus_time_ns = 0U;
    g_boot_service_count = 0U;

    for (size_t i = 0; i < AUTOMOTIVE_INTERNAL_QUEUE_SLOTS; ++i) {
        g_queues[i].in_use = 0U;
    }

    for (size_t i = 0; i < BHARAT_AUTOMOTIVE_MAX_WATCHDOGS; ++i) {
        g_watchdogs[i].armed = 0U;
    }

    for (size_t i = 0; i <= AUTOMOTIVE_FAULT_DOMAIN_GENERIC; ++i) {
        g_domain_health[i] = AUTOMOTIVE_HEALTH_OK;
    }
}

void subsys_automotive_set_runtime_policy(const automotive_runtime_policy_t* policy) {
    if (!policy) return;
    g_policy = *policy;
}

const automotive_runtime_policy_t* subsys_automotive_get_runtime_policy(void) {
    return &g_policy;
}

bool subsys_automotive_queue_register(uint32_t queue_id, const automotive_bounded_queue_t* queue_cfg) {
    if (!queue_cfg || queue_cfg->capacity == 0U || queue_cfg->capacity > BHARAT_AUTOMOTIVE_MAX_QUEUE_DEPTH) {
        return false;
    }

    for (size_t i = 0; i < AUTOMOTIVE_INTERNAL_QUEUE_SLOTS; ++i) {
        if (g_queues[i].in_use && g_queues[i].queue_id == queue_id) {
            g_queues[i].cfg = *queue_cfg;
            g_queues[i].cfg.current_depth = 0U;
            return true;
        }
    }

    for (size_t i = 0; i < AUTOMOTIVE_INTERNAL_QUEUE_SLOTS; ++i) {
        if (!g_queues[i].in_use) {
            g_queues[i].in_use = 1U;
            g_queues[i].queue_id = queue_id;
            g_queues[i].cfg = *queue_cfg;
            g_queues[i].cfg.current_depth = 0U;
            return true;
        }
    }

    return false;
}

bool subsys_automotive_queue_push(uint32_t queue_id, uint8_t producer_priority) {
    queue_slot_t* slot = find_queue(queue_id);
    if (!slot) return false;

    if (slot->cfg.current_depth >= slot->cfg.capacity) {
        return false;
    }

    if (slot->cfg.priority_aware_wakeup && producer_priority > slot->cfg.high_watermark) {
        slot->cfg.high_watermark = producer_priority;
    }

    slot->cfg.current_depth++;
    return true;
}

bool subsys_automotive_queue_pop(uint32_t queue_id, uint8_t consumer_priority) {
    queue_slot_t* slot = find_queue(queue_id);
    if (!slot || slot->cfg.current_depth == 0U) return false;

    if (slot->cfg.priority_aware_wakeup && consumer_priority > slot->cfg.high_watermark) {
        slot->cfg.high_watermark = consumer_priority;
    }

    slot->cfg.current_depth--;
    return true;
}

void subsys_automotive_register_deadline_hook(automotive_deadline_hook_t hook) {
    g_deadline_hook = hook;
}

void subsys_automotive_emit_deadline_event(const automotive_deadline_event_t* event) {
    if (g_deadline_hook && event) {
        g_deadline_hook(event);
    }
}

bool subsys_automotive_watchdog_arm(const automotive_watchdog_t* watchdog) {
    if (!watchdog) return false;

    for (size_t i = 0; i < BHARAT_AUTOMOTIVE_MAX_WATCHDOGS; ++i) {
        if (!g_watchdogs[i].armed || g_watchdogs[i].watchdog_id == watchdog->watchdog_id) {
            g_watchdogs[i] = *watchdog;
            g_watchdogs[i].armed = 1U;
            return true;
        }
    }

    return false;
}

bool subsys_automotive_watchdog_kick(uint32_t watchdog_id, uint64_t current_tick) {
    for (size_t i = 0; i < BHARAT_AUTOMOTIVE_MAX_WATCHDOGS; ++i) {
        if (g_watchdogs[i].armed && g_watchdogs[i].watchdog_id == watchdog_id) {
            g_watchdogs[i].last_kick_tick = current_tick;
            return true;
        }
    }
    return false;
}

bool subsys_automotive_watchdog_poll(uint64_t current_tick, automotive_health_report_t* out_report) {
    for (size_t i = 0; i < BHARAT_AUTOMOTIVE_MAX_WATCHDOGS; ++i) {
        if (!g_watchdogs[i].armed) continue;

        uint64_t elapsed = current_tick - g_watchdogs[i].last_kick_tick;
        if (elapsed > (uint64_t)g_watchdogs[i].timeout_ms) {
            automotive_fault_domain_t domain = g_watchdogs[i].domain;
            if (domain <= AUTOMOTIVE_FAULT_DOMAIN_GENERIC) {
                g_domain_health[domain] = AUTOMOTIVE_HEALTH_FAILED;
            }

            if (out_report) {
                out_report->subsystem_id = g_watchdogs[i].watchdog_id;
                out_report->domain = domain;
                out_report->state = AUTOMOTIVE_HEALTH_FAILED;
            }
            return true;
        }
    }

    return false;
}

void subsys_automotive_report_health(const automotive_health_report_t* report) {
    if (!report || report->domain > AUTOMOTIVE_FAULT_DOMAIN_GENERIC) return;
    g_domain_health[report->domain] = report->state;
}

automotive_health_state_t subsys_automotive_get_domain_health(automotive_fault_domain_t domain) {
    if (domain > AUTOMOTIVE_FAULT_DOMAIN_GENERIC) {
        return AUTOMOTIVE_HEALTH_FAILED;
    }
    return g_domain_health[domain];
}

void subsys_automotive_register_time_sync_hook(automotive_time_sync_hook_t hook) {
    g_time_sync_hook = hook;
}

void subsys_automotive_emit_time_sync(uint64_t monotonic_time_ns, uint64_t bus_time_ns) {
    g_last_monotonic_time_ns = monotonic_time_ns;
    g_last_bus_time_ns = bus_time_ns;

    if (g_time_sync_hook) {
        g_time_sync_hook(monotonic_time_ns, bus_time_ns);
    }
}

bool subsys_automotive_get_last_time_sync(uint64_t* monotonic_time_ns, uint64_t* bus_time_ns) {
    if (!monotonic_time_ns || !bus_time_ns || g_last_monotonic_time_ns == 0U) {
        return false;
    }

    *monotonic_time_ns = g_last_monotonic_time_ns;
    *bus_time_ns = g_last_bus_time_ns;
    return true;
}

bool subsys_automotive_send_frame(automotive_fieldbus_t bus, uint32_t id, const uint8_t* data, uint8_t dlc) {
    if (!data || dlc == 0U) return false;

    if (bus == AUTOMOTIVE_BUS_CAN_CLASSIC && dlc > 8U) return false;
    if (bus == AUTOMOTIVE_BUS_CAN_FD && dlc > 64U) return false;
    if (bus == AUTOMOTIVE_BUS_LIN && (id > 0x3FU || dlc > 8U)) return false;

    (void)id;
    return true;
}

bool subsys_automotive_send_lin_frame(uint8_t frame_id, const uint8_t* data, uint8_t dlc) {
    return subsys_automotive_send_frame(AUTOMOTIVE_BUS_LIN, (uint32_t)frame_id, data, dlc);
}

void subsys_automotive_register_ethernet_hook(automotive_ethernet_hook_t hook) {
    g_ethernet_hook = hook;
}

bool subsys_automotive_send_ethernet_frame(uint16_t ethertype,
                                           const uint8_t* payload,
                                           uint16_t payload_len,
                                           uint8_t traffic_class) {
    if (!payload || payload_len == 0U) {
        return false;
    }

    if (!g_ethernet_hook) {
        return false;
    }

    return g_ethernet_hook(ethertype, payload, payload_len, traffic_class);
}

void subsys_automotive_select_boot_profile(automotive_boot_profile_t profile) {
    g_policy.boot_profile = profile;
}

automotive_boot_profile_t subsys_automotive_get_boot_profile(void) {
    return g_policy.boot_profile;
}

bool subsys_automotive_register_boot_service(const automotive_boot_service_t* service) {
    if (!service || g_boot_service_count >= BHARAT_AUTOMOTIVE_MAX_BOOT_SERVICES) {
        return false;
    }

    g_boot_services[g_boot_service_count++] = *service;
    return true;
}

size_t subsys_automotive_plan_boot_stage(uint32_t stage_id, uint32_t* out_service_ids, size_t max_services) {
    size_t emitted = 0U;

    if (!out_service_ids || max_services == 0U) {
        return 0U;
    }

    for (size_t i = 0; i < g_boot_service_count && emitted < max_services; ++i) {
        const automotive_boot_service_t* service = &g_boot_services[i];

        if (g_policy.boot_profile == AUTOMOTIVE_BOOT_PROFILE_RT_MINIMAL &&
            !service->essential_for_minimal_lane) {
            continue;
        }

        if (service->dependency_count == stage_id) {
            out_service_ids[emitted++] = service->service_id;
        }
    }

    return emitted;
}

size_t subsys_automotive_run_boot_stage(uint32_t stage_id,
                                        automotive_boot_service_start_hook_t start_hook,
                                        uint32_t* out_service_ids,
                                        size_t max_services) {
    size_t planned = subsys_automotive_plan_boot_stage(stage_id, out_service_ids, max_services);

    if (!start_hook) {
        return planned;
    }

    for (size_t i = 0; i < planned; ++i) {
        start_hook(out_service_ids[i]);
    }

    return planned;
}

bool subsys_automotive_send_can_frame(uint32_t id, const uint8_t* data, uint8_t dlc) {
    return subsys_automotive_send_frame(AUTOMOTIVE_BUS_CAN_CLASSIC, id, data, dlc);
}

void bharat_rt_deadline_timeout_hook(uint32_t endpoint_ref, uint32_t request_id, uint64_t current_ticks) {
    automotive_deadline_event_t event;

    event.queue_id = endpoint_ref;
    event.producer_prio = 0U;
    event.consumer_prio = 0U;
    event.absolute_deadline_us = current_ticks;

    (void)request_id;
    subsys_automotive_emit_deadline_event(&event);
}
