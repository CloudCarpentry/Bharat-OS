#include "tests/ktest.h"
#include "device/irq_domain.h"
#include "boot/boot_selftest.h"

static bool test_irq_domain_create_and_map(void) {
    irq_domain_t* domain = irq_domain_create("ktest-root", 32U, 64U, NULL);
    uint32_t virq = 0U;
    int ret;

    KTEST_ASSERT(domain != NULL, "irq_domain_create returned NULL");

    ret = irq_domain_map(domain, 40U, 7U);
    KTEST_ASSERT(ret == 0, "irq_domain_map failed for valid mapping");

    ret = irq_domain_translate(domain, 7U, &virq);
    KTEST_ASSERT(ret == 0, "irq_domain_translate failed for mapped hwirq");
    KTEST_ASSERT(virq == 40U, "Translated virq mismatch");

    return true;
}

static bool test_irq_domain_rejects_invalid_and_duplicate_map(void) {
    irq_domain_t* domain = irq_domain_create("ktest-dup", 128U, 16U, NULL);
    int ret;

    KTEST_ASSERT(domain != NULL, "irq_domain_create for duplicate test failed");

    ret = irq_domain_map(domain, 127U, 1U);
    KTEST_ASSERT(ret < 0, "irq_domain_map should reject virq below domain range");

    ret = irq_domain_map(domain, 128U, 1U);
    KTEST_ASSERT(ret == 0, "initial irq_domain_map failed");

    ret = irq_domain_map(domain, 128U, 2U);
    KTEST_ASSERT(ret < 0, "duplicate virq should be rejected");

    ret = irq_domain_map(domain, 129U, 1U);
    KTEST_ASSERT(ret < 0, "duplicate hwirq in same domain should be rejected");

    return true;
}

static bool test_irq_domain_unmap_removes_translation(void) {
    irq_domain_t* domain = irq_domain_create("ktest-unmap", 200U, 8U, NULL);
    uint32_t virq = 0U;
    int ret;

    KTEST_ASSERT(domain != NULL, "irq_domain_create for unmap test failed");

    ret = irq_domain_map(domain, 203U, 5U);
    KTEST_ASSERT(ret == 0, "irq_domain_map failed before unmap");

    ret = irq_domain_unmap(domain, 203U);
    KTEST_ASSERT(ret == 0, "irq_domain_unmap failed");

    ret = irq_domain_translate(domain, 5U, &virq);
    KTEST_ASSERT(ret < 0, "translation should fail after unmap");

    return true;
}

static ktest_case_t irq_domain_tests[] = {
    {"Domain create/map/translate", test_irq_domain_create_and_map},
    {"Domain duplicate/range checks", test_irq_domain_rejects_invalid_and_duplicate_map},
    {"Domain unmap behavior", test_irq_domain_unmap_removes_translation},
};

int boot_test_irq_domain(void) {
    ktest_run_suite("IRQ Domain Unit Tests", irq_domain_tests, 3);
    if (!test_irq_domain_create_and_map()) return -1;
    if (!test_irq_domain_rejects_invalid_and_duplicate_map()) return -1;
    if (!test_irq_domain_unmap_removes_translation()) return -1;
    return 0;
}

REGISTER_BOOT_SELFTEST("irq_domain", "interrupt", boot_test_irq_domain, BOOT_TEST_STAGE_RUNTIME, BOOT_TEST_MANDATORY, 0, true)
