#ifndef BHARAT_SUBSYS_AUTOMOTIVE_H
#define BHARAT_SUBSYS_AUTOMOTIVE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define BHARAT_AUTOMOTIVE_MAX_QUEUE_DEPTH 64U
#define BHARAT_AUTOMOTIVE_MAX_WATCHDOGS 16U
#define BHARAT_AUTOMOTIVE_MAX_BOOT_SERVICES 24U

typedef enum {
    AUTOMOTIVE_IPC_MODE_BEST_EFFORT = 0,
    AUTOMOTIVE_IPC_MODE_DETERMINISTIC = 1
} automotive_ipc_mode_t;

typedef enum {
    AUTOMOTIVE_BOOT_PROFILE_NORMAL = 0,
    AUTOMOTIVE_BOOT_PROFILE_SAFE_MODE = 1,
    AUTOMOTIVE_BOOT_PROFILE_RT_MINIMAL = 2
} automotive_boot_profile_t;

typedef enum {
    AUTOMOTIVE_FAULT_DOMAIN_POWERTRAIN = 0,
    AUTOMOTIVE_FAULT_DOMAIN_CHASSIS = 1,
    AUTOMOTIVE_FAULT_DOMAIN_INFOTAINMENT = 2,
    AUTOMOTIVE_FAULT_DOMAIN_AUTONOMY = 3,
    AUTOMOTIVE_FAULT_DOMAIN_INDUSTRIAL_IO = 4,
    AUTOMOTIVE_FAULT_DOMAIN_GENERIC = 5
} automotive_fault_domain_t;

typedef enum {
    AUTOMOTIVE_HEALTH_OK = 0,
    AUTOMOTIVE_HEALTH_DEGRADED = 1,
    AUTOMOTIVE_HEALTH_FAILED = 2
} automotive_health_state_t;

typedef enum {
    AUTOMOTIVE_BUS_CAN_CLASSIC = 0,
    AUTOMOTIVE_BUS_CAN_FD = 1,
    AUTOMOTIVE_BUS_LIN = 2
} automotive_fieldbus_t;

typedef struct {
    uint16_t capacity;
    uint16_t high_watermark;
    uint16_t current_depth;
    uint32_t max_enqueue_latency_us;
    uint8_t lock_avoidance;
    uint8_t priority_aware_wakeup;
} automotive_bounded_queue_t;

typedef struct {
    uint32_t queue_id;
    uint8_t producer_prio;
    uint8_t consumer_prio;
    uint64_t absolute_deadline_us;
} automotive_deadline_event_t;

typedef struct {
    uint32_t watchdog_id;
    uint32_t timeout_ms;
    uint64_t last_kick_tick;
    automotive_fault_domain_t domain;
    uint8_t armed;
} automotive_watchdog_t;

typedef struct {
    uint32_t subsystem_id;
    automotive_fault_domain_t domain;
    automotive_health_state_t state;
} automotive_health_report_t;

typedef struct {
    uint32_t service_id;
    uint32_t dependency_count;
    uint32_t dependencies[4];
    uint8_t essential_for_minimal_lane;
} automotive_boot_service_t;

typedef struct {
    automotive_ipc_mode_t ipc_mode;
    automotive_boot_profile_t boot_profile;
    uint32_t default_queue_budget_us;
    uint32_t default_deadline_slack_us;
} automotive_runtime_policy_t;

typedef void (*automotive_deadline_hook_t)(const automotive_deadline_event_t* event);
typedef void (*automotive_time_sync_hook_t)(uint64_t monotonic_time_ns, uint64_t bus_time_ns);
typedef bool (*automotive_ethernet_hook_t)(uint16_t ethertype,
                                           const uint8_t* payload,
                                           uint16_t payload_len,
                                           uint8_t traffic_class);
typedef void (*automotive_boot_service_start_hook_t)(uint32_t service_id);

/* Initialize the automotive/robotics/industrial subsystem. */
void subsys_automotive_init(void);

/* Configure deterministic IPC mode and baseline runtime policy. */
void subsys_automotive_set_runtime_policy(const automotive_runtime_policy_t* policy);
const automotive_runtime_policy_t* subsys_automotive_get_runtime_policy(void);

/* Bounded latency queue controls. */
bool subsys_automotive_queue_register(uint32_t queue_id, const automotive_bounded_queue_t* queue_cfg);
bool subsys_automotive_queue_push(uint32_t queue_id, uint8_t producer_priority);
bool subsys_automotive_queue_pop(uint32_t queue_id, uint8_t consumer_priority);

/* Deadline-aware scheduling hooks. */
void subsys_automotive_register_deadline_hook(automotive_deadline_hook_t hook);
void subsys_automotive_emit_deadline_event(const automotive_deadline_event_t* event);

/* Safety partitioning / health monitoring and watchdog framework. */
bool subsys_automotive_watchdog_arm(const automotive_watchdog_t* watchdog);
bool subsys_automotive_watchdog_kick(uint32_t watchdog_id, uint64_t current_tick);
bool subsys_automotive_watchdog_poll(uint64_t current_tick, automotive_health_report_t* out_report);
void subsys_automotive_report_health(const automotive_health_report_t* report);
automotive_health_state_t subsys_automotive_get_domain_health(automotive_fault_domain_t domain);

/* Automotive field bus and time sync hooks. */
void subsys_automotive_register_time_sync_hook(automotive_time_sync_hook_t hook);
void subsys_automotive_emit_time_sync(uint64_t monotonic_time_ns, uint64_t bus_time_ns);
bool subsys_automotive_get_last_time_sync(uint64_t* monotonic_time_ns, uint64_t* bus_time_ns);
bool subsys_automotive_send_frame(automotive_fieldbus_t bus, uint32_t id, const uint8_t* data, uint8_t dlc);
bool subsys_automotive_send_lin_frame(uint8_t frame_id, const uint8_t* data, uint8_t dlc);

/* Automotive Ethernet hook seam for later TSN integration. */
void subsys_automotive_register_ethernet_hook(automotive_ethernet_hook_t hook);
bool subsys_automotive_send_ethernet_frame(uint16_t ethertype,
                                           const uint8_t* payload,
                                           uint16_t payload_len,
                                           uint8_t traffic_class);

/* Fast boot subsystem support: staged init, dependencies, and minimal lane. */
void subsys_automotive_select_boot_profile(automotive_boot_profile_t profile);
automotive_boot_profile_t subsys_automotive_get_boot_profile(void);
bool subsys_automotive_register_boot_service(const automotive_boot_service_t* service);
size_t subsys_automotive_plan_boot_stage(uint32_t stage_id, uint32_t* out_service_ids, size_t max_services);
size_t subsys_automotive_run_boot_stage(uint32_t stage_id,
                                        automotive_boot_service_start_hook_t start_hook,
                                        uint32_t* out_service_ids,
                                        size_t max_services);

/* Backward compatible API. */
bool subsys_automotive_send_can_frame(uint32_t id, const uint8_t* data, uint8_t dlc);

#endif // BHARAT_SUBSYS_AUTOMOTIVE_H
