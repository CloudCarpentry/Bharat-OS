#include "../../include/kernel/status.h"
#include "../../include/mm/accel_mem.h"
#include "../../include/mm/pmm.h"

// Basic assertion macro suitable for host test runner
extern int kprintf(const char *fmt, ...);
#define ASSERT_OK(x) do { if ((x) != K_OK) { return -1; } } while(0)
#define ASSERT_TRUE(x) do { if (!(x)) { return -1; } } while(0)

int test_accel_pin_unpin_lifecycle(void) {
    accel_buffer_t *buf = NULL;
    int ret = accel_mem_buffer_create(8192, PAGE_SIZE, ACCEL_MEM_SHARED, (cap_handle_t){0}, &buf);
    ASSERT_OK(ret);
    ASSERT_TRUE(buf != NULL);
    ASSERT_TRUE(buf->state == ACCEL_BUF_STATE_CREATED);

    ret = accel_mem_pin(buf);
    ASSERT_OK(ret);
    ASSERT_TRUE(buf->state == ACCEL_BUF_STATE_PINNED);
    ASSERT_TRUE(buf->pin_count == 1);

    ret = accel_mem_pin(buf);
    ASSERT_OK(ret);
    ASSERT_TRUE(buf->pin_count == 2);

    ret = accel_mem_unpin(buf);
    ASSERT_OK(ret);
    ASSERT_TRUE(buf->pin_count == 1);
    ASSERT_TRUE(buf->state == ACCEL_BUF_STATE_PINNED);

    ret = accel_mem_unpin(buf);
    ASSERT_OK(ret);
    ASSERT_TRUE(buf->pin_count == 0);
    ASSERT_TRUE(buf->state == ACCEL_BUF_STATE_CREATED);

    accel_mem_buffer_destroy(buf);
    return 0; // PASS
}

int test_accel_over_budget_pin_rejection(void) {
    accel_buffer_t *buf1 = NULL;
    size_t big_size = 1024ULL * 1024ULL * 600ULL;
    int ret = accel_mem_buffer_create(big_size, PAGE_SIZE, ACCEL_MEM_SHARED, (cap_handle_t){0}, &buf1);
    ASSERT_OK(ret);

    ret = accel_mem_pin(buf1);
    ASSERT_TRUE(ret == K_ERR_PMM_EXHAUSTED);
    ASSERT_TRUE(buf1->state == ACCEL_BUF_STATE_CREATED);

    accel_mem_buffer_destroy(buf1);
    return 0;
}

int test_accel_sg_generation(void) {
    accel_buffer_t *buf = NULL;
    int ret = accel_mem_buffer_create(8192, PAGE_SIZE, ACCEL_MEM_SHARED, (cap_handle_t){0}, &buf);
    ASSERT_OK(ret);

    ret = accel_mem_pin(buf);
    ASSERT_OK(ret);

    buf->phys_pages[0] = 0x1000;
    buf->phys_pages[1] = 0x2000;

    ret = accel_sg_build(buf);
    ASSERT_OK(ret);
    ASSERT_TRUE(buf->state == ACCEL_BUF_STATE_SG_BUILT);
    ASSERT_TRUE(buf->sg_table != NULL);
    ASSERT_TRUE(buf->sg_table->num_entries == 1);
    ASSERT_TRUE(buf->sg_table->head->length == 8192);

    accel_sg_release(buf);
    ASSERT_TRUE(buf->state == ACCEL_BUF_STATE_PINNED);
    ASSERT_TRUE(buf->sg_table == NULL);

    accel_mem_buffer_destroy(buf);
    return 0;
}

extern int test_virt_accel_smoke(void);

int ktest_accel_lifecycle_run(void) {
    int failures = 0;

    if (test_accel_pin_unpin_lifecycle() != 0) failures++;
    if (test_accel_over_budget_pin_rejection() != 0) failures++;
    if (test_accel_sg_generation() != 0) failures++;

    // Also call the smoke test directly
    if (test_virt_accel_smoke() != 0) failures++;

    return failures;
}
