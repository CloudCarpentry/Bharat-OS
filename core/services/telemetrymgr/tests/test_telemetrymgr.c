#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "../registry.h"
#include <bharat/uapi/service_status.h>

void bh_time_get(uint32_t clock_id, uint64_t *time_ns) {
    (void)clock_id;
    *time_ns = 123456789;
}

void test_registry_init() {
    metrics_registry_t reg;
    registry_init(&reg);
    assert(reg.active_count == 0);
    assert(reg.overflow_count == 0);
    printf("test_registry_init passed\n");
}

void test_register_metric() {
    metrics_registry_t reg;
    registry_init(&reg);
    bharat_counter_desc_t desc = {
        .counter_id = 1,
        .source_id = 1,
        .type = BHARAT_COUNTER_TYPE_MONOTONIC,
        .name = "test_counter",
        .unit = "ops"
    };
    uint32_t cid;
    int32_t ret = registry_register_metric(&reg, &desc, &cid);
    assert(ret == BHARAT_STATUS_OK);
    assert(cid == 1);
    assert(reg.active_count == 1);

    // duplicate
    ret = registry_register_metric(&reg, &desc, &cid);
    assert(ret == BHARAT_STATUS_ERR_ALREADY_EXISTS);
    printf("test_register_metric passed\n");
}

void test_update_counter() {
    metrics_registry_t reg;
    registry_init(&reg);
    bharat_counter_desc_t desc = {
        .counter_id = 1,
        .type = BHARAT_COUNTER_TYPE_MONOTONIC,
    };
    uint32_t cid;
    registry_register_metric(&reg, &desc, &cid);

    int32_t ret = registry_update_counter(&reg, cid, 10, 1);
    assert(ret == BHARAT_STATUS_OK);

    bharat_counter_snapshot_t snap;
    registry_query_metric(&reg, cid, &snap);
    assert(snap.value == 10);

    // Stale sequence
    ret = registry_update_counter(&reg, cid, 20, 1);
    assert(ret == BHARAT_STATUS_ERR_BAD_STATE);

    // Counter overflow
    ret = registry_update_counter(&reg, cid, 5, 2);
    assert(ret == BHARAT_STATUS_ERR_INVALID_ARG);

    printf("test_update_counter passed\n");
}

void test_capacity_exhaustion() {
    metrics_registry_t reg;
    registry_init(&reg);
    for (int i=0; i<MAX_METRICS; i++) {
        bharat_counter_desc_t desc = {
            .counter_id = i + 1,
            .type = BHARAT_COUNTER_TYPE_MONOTONIC,
        };
        uint32_t cid;
        assert(registry_register_metric(&reg, &desc, &cid) == BHARAT_STATUS_OK);
    }

    bharat_counter_desc_t desc = {
        .counter_id = MAX_METRICS + 1,
        .type = BHARAT_COUNTER_TYPE_MONOTONIC,
    };
    uint32_t cid;
    assert(registry_register_metric(&reg, &desc, &cid) == BHARAT_STATUS_ERR_NO_MEMORY);
    assert(reg.overflow_count == 1);
    printf("test_capacity_exhaustion passed\n");
}

void test_update_gauge() {
    metrics_registry_t reg;
    registry_init(&reg);
    bharat_counter_desc_t desc = {
        .counter_id = 1,
        .type = BHARAT_COUNTER_TYPE_GAUGE,
    };
    uint32_t cid;
    registry_register_metric(&reg, &desc, &cid);

    int32_t ret = registry_update_gauge(&reg, cid, 50, 1);
    assert(ret == BHARAT_STATUS_OK);

    bharat_counter_snapshot_t snap;
    registry_query_metric(&reg, cid, &snap);
    assert(snap.value == 50);

    // Stale sequence
    ret = registry_update_gauge(&reg, cid, 20, 1);
    assert(ret == BHARAT_STATUS_ERR_BAD_STATE);

    printf("test_update_gauge passed\n");
}

void test_snapshot_query() {
    metrics_registry_t reg;
    registry_init(&reg);

    bharat_counter_desc_t desc1 = { .counter_id = 1, .type = BHARAT_COUNTER_TYPE_GAUGE };
    bharat_counter_desc_t desc2 = { .counter_id = 2, .type = BHARAT_COUNTER_TYPE_MONOTONIC };
    uint32_t cid;
    registry_register_metric(&reg, &desc1, &cid);
    registry_register_metric(&reg, &desc2, &cid);

    registry_update_gauge(&reg, 1, 100, 1);
    registry_update_counter(&reg, 2, 200, 2);

    bharat_counter_snapshot_t snap;
    assert(registry_query_metric(&reg, 1, &snap) == BHARAT_STATUS_OK);
    assert(snap.value == 100);

    assert(registry_query_metric(&reg, 2, &snap) == BHARAT_STATUS_OK);
    assert(snap.value == 200);

    printf("test_snapshot_query passed\n");
}

void test_invalid_messages() {
    metrics_registry_t reg;
    registry_init(&reg);

    // Registering a NULL desc
    uint32_t cid;
    assert(registry_register_metric(&reg, NULL, &cid) == BHARAT_STATUS_ERR_INVALID_ARG);

    printf("test_invalid_messages passed\n");
}

void test_shutdown() {
    printf("test_shutdown passed\n");
}

int main() {
    test_registry_init();
    test_register_metric();
    test_update_counter();
    test_capacity_exhaustion();
    test_update_gauge();
    test_snapshot_query();
    test_invalid_messages();
    test_shutdown();
    printf("All registry tests passed.\n");
    return 0;
}
