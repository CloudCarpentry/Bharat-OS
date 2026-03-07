#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "../kernel/include/advanced/ai_sched.h"
#include "../kernel/include/advanced/multikernel.h"

// Mock for urpc_send to capture sent messages instead of actually sending
static urpc_msg_t last_sent_message = {0};
static int urpc_send_called = 0;

int urpc_send(urpc_ring_t* ring, const urpc_msg_t* msg) {
    urpc_send_called++;
    last_sent_message = *msg;
    return 0;
}

// Expose the real governor function for testing
extern void ai_governor_suggest_action(uint32_t thread_id, kernel_telemetry_t* telemetry, ai_heuristic_config_t* config, urpc_ring_t* control_ring);

int main() {
    printf("Running AI Governor Integration Tests...\n");

    ai_heuristic_config_t config = {
        .penalty_threshold = 5000,
        .weight_ipc_latency = 2,
        .weight_cache_miss = 10
    };

    urpc_ring_t mock_ring = {0};

    // Test Case 1: High Penalty Score triggers AI_ACTION_MIGRATE_TASK
    kernel_telemetry_t telemetry_high_penalty = {
        .cpu_usage_pct = 45,
        .ipc_latency_ns = 2000,
        .cache_miss_rate = 150,
        .context_switches = 50,
        .numa_node_id = 1
    };

    urpc_send_called = 0;
    ai_governor_suggest_action(1001, &telemetry_high_penalty, &config, &mock_ring);

    assert(urpc_send_called == 1);
    assert(last_sent_message.msg_type == AI_MSG_TYPE_SUGGESTION);
    assert(last_sent_message.payload_size == sizeof(ai_suggestion_t));

    ai_suggestion_t* sent_suggestion_1 = (ai_suggestion_t*)last_sent_message.payload_data;
    assert(sent_suggestion_1->action == AI_ACTION_MIGRATE_TASK);
    assert(sent_suggestion_1->target_id == 1001);
    assert(sent_suggestion_1->value == 0); // Should migrate from 1 to 0


    // Test Case 2: High CPU Usage triggers AI_ACTION_ADJUST_PRIORITY
    kernel_telemetry_t telemetry_high_cpu = {
        .cpu_usage_pct = 85,
        .ipc_latency_ns = 500,
        .cache_miss_rate = 20,
        .context_switches = 10,
        .numa_node_id = 0
    };

    urpc_send_called = 0;
    ai_governor_suggest_action(1002, &telemetry_high_cpu, &config, &mock_ring);

    assert(urpc_send_called == 1);
    assert(last_sent_message.msg_type == AI_MSG_TYPE_SUGGESTION);
    assert(last_sent_message.payload_size == sizeof(ai_suggestion_t));

    ai_suggestion_t* sent_suggestion_2 = (ai_suggestion_t*)last_sent_message.payload_data;
    assert(sent_suggestion_2->action == AI_ACTION_ADJUST_PRIORITY);
    assert(sent_suggestion_2->target_id == 1002);
    assert(sent_suggestion_2->value == 1);

    // Test Case 3: Normal conditions trigger no action
    kernel_telemetry_t telemetry_normal = {
        .cpu_usage_pct = 30,
        .ipc_latency_ns = 500,
        .cache_miss_rate = 20,
        .context_switches = 10,
        .numa_node_id = 0
    };

    urpc_send_called = 0;
    ai_governor_suggest_action(1003, &telemetry_normal, &config, &mock_ring);
    assert(urpc_send_called == 0);

    printf("AI Governor Integration Tests passed successfully.\n");
    return 0;
}
