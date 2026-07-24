#include "../../include/tests/ktest.h"
#include "../../include/mm/prot_domain.h"
#include "../../include/arch/arch_caps.h"
#include "../../include/hal/hal.h"
#include "../../include/hal/hal_pt.h"

static bool test_prot_domain_create_destroy(void) {
    prot_domain_t* domain = prot_domain_create();
    KTEST_ASSERT(domain != NULL, "prot_domain_create should succeed");

    arch_caps_t caps = arch_get_caps();
    if (arch_caps_test(caps, ARCH_CAP_MMU_FULL)) {
        KTEST_ASSERT(domain->mode == PROT_MODE_MMU_FULL, "Profile matches PROT_MODE_MMU_FULL");
    } else if (arch_caps_test(caps, ARCH_CAP_MMU_LITE)) {
        KTEST_ASSERT(domain->mode == PROT_MODE_MMU_LITE, "Profile matches PROT_MODE_MMU_LITE");
    } else if (arch_caps_test(caps, ARCH_CAP_MPU_ONLY)) {
        KTEST_ASSERT(domain->mode == PROT_MODE_MPU_ONLY, "Profile matches PROT_MODE_MPU_ONLY");
    } else {
        KTEST_ASSERT(domain->mode == PROT_MODE_NONE, "Profile matches PROT_MODE_NONE");
    }

    prot_domain_destroy(domain);
    return true;
}

static bool test_mpu_exhaustion(void) {
    arch_caps_t caps = arch_get_caps();
    if (!arch_caps_test(caps, ARCH_CAP_MPU_ONLY)) {
        return true; // Skip if not MPU
    }

    prot_domain_t* domain = prot_domain_create();
    KTEST_ASSERT(domain != NULL, "Domain create failed");

    int i = 0;
    while(1) {
        int ret = prot_domain_map_region(domain, i * 4096, i * 4096, 4096, 0);
        if (ret < 0) {
            KTEST_ASSERT(i == 16, "MPU should exhaust after exactly 16 regions for the demo backend");
            break;
        }
        i++;
    }
    prot_domain_destroy(domain);
    return true;
}

static bool test_mpu_overlap(void) {
    arch_caps_t caps = arch_get_caps();
    if (!arch_caps_test(caps, ARCH_CAP_MPU_ONLY)) {
        return true; // Skip if not MPU
    }

    prot_domain_t* domain = prot_domain_create();
    KTEST_ASSERT(domain != NULL, "Domain create failed");

    int ret = prot_domain_map_region(domain, 0x1000, 0x1000, 0x2000, 0);
    KTEST_ASSERT(ret == 0, "Map region 1 failed");

    ret = prot_domain_map_region(domain, 0x2000, 0x2000, 0x1000, 0);
    KTEST_ASSERT(ret < 0, "Map overlapping region 2 should fail");

    prot_domain_destroy(domain);
    return true;
}

static bool test_mmu_sparse_mapping(void) {
    arch_caps_t caps = arch_get_caps();
    if (!arch_caps_test(caps, ARCH_CAP_MMU_FULL)) {
        return true; // Skip if not MMU
    }

    prot_domain_t* domain = prot_domain_create();
    KTEST_ASSERT(domain != NULL, "Domain create failed");

    // Test 1: Map low VA
    uintptr_t low_vaddr = 0x0000000040000000ULL; // 1 GiB
    int ret = prot_domain_map_region(domain, low_vaddr, 0x10000000, 4096, HAL_PT_FLAG_READ);
    KTEST_ASSERT(ret == 0, "Low VA mapping failed");

    // Test 2: Map sparse high lower-half VA (512 GiB)
    uintptr_t high_vaddr = 0x0000002000000000ULL;
    ret = prot_domain_map_region(domain, high_vaddr, 0x80000000, 4096, HAL_PT_FLAG_READ);
    KTEST_ASSERT(ret == 0, "Sparse high VA mapping failed");

    // Test 3: Map two sparse VAs far apart
    uintptr_t very_high_vaddr = 0x0000003F00000000ULL;
    ret = prot_domain_map_region(domain, very_high_vaddr, 0x90000000, 4096, HAL_PT_FLAG_READ);
    KTEST_ASSERT(ret == 0, "Very high sparse VA mapping failed");

    // Test 4: Query and verify
    uintptr_t paddr = 0;
    ret = prot_domain_query_region(domain, high_vaddr, &paddr, NULL);
    KTEST_ASSERT(ret == 0, "Sparse query failed");
    KTEST_ASSERT(paddr == 0x80000000, "Sparse query address mismatch");

    ret = prot_domain_query_region(domain, very_high_vaddr, &paddr, NULL);
    KTEST_ASSERT(ret == 0, "Very high sparse query failed");
    KTEST_ASSERT(paddr == 0x90000000, "Very high sparse query address mismatch");

    // Test 5: Unmap and Verify
    ret = prot_domain_unmap_region(domain, high_vaddr, 4096);
    KTEST_ASSERT(ret == 0, "Unmap sparse VA failed");
    ret = prot_domain_query_region(domain, high_vaddr, &paddr, NULL);
    KTEST_ASSERT(ret != 0, "Query after unmap should fail");

    // Test 6: Protect sparse VA
    ret = prot_domain_protect_region(domain, very_high_vaddr, 4096, HAL_PT_FLAG_READ | HAL_PT_FLAG_WRITE);
    KTEST_ASSERT(ret == 0, "Protect sparse VA failed");
    uint32_t flags = 0;
    ret = prot_domain_query_region(domain, very_high_vaddr, &paddr, &flags);
    KTEST_ASSERT(ret == 0, "Query after protect failed");
    KTEST_ASSERT(flags & HAL_PT_FLAG_WRITE, "Protect flags not updated");

    // Test 6.1: Remap sparse VA
    ret = prot_domain_map_region(domain, very_high_vaddr, 0xA0000000, 4096, HAL_PT_FLAG_READ);
    KTEST_ASSERT(ret == 0, "Remap sparse VA failed");
    ret = prot_domain_query_region(domain, very_high_vaddr, &paddr, &flags);
    KTEST_ASSERT(ret == 0, "Query after remap failed");
    KTEST_ASSERT(paddr == 0xA0000000, "Remap address mismatch");

    // Test 7: Reject non-canonical VA
    uintptr_t non_canonical = 0x0000800000000000ULL;
    ret = prot_domain_map_region(domain, non_canonical, 0x10000000, 4096, HAL_PT_FLAG_READ);
    KTEST_ASSERT(ret != 0, "Non-canonical mapping should be rejected");

    // Test 8: Reject unmapped unprotect/unmap
    ret = prot_domain_unmap_region(domain, 0x0000001234567000ULL, 4096);
    KTEST_ASSERT(ret != 0, "Unmap of unmapped region should fail");

    prot_domain_destroy(domain);
    return true;
}

static bool test_prot_none_fail_fast(void) {
    arch_caps_t caps = arch_get_caps();
    if (arch_caps_test(caps, ARCH_CAP_MMU_FULL) || arch_caps_test(caps, ARCH_CAP_MMU_LITE) || arch_caps_test(caps, ARCH_CAP_MPU_ONLY)) {
        return true; // Skip if supported
    }

    prot_domain_t* domain = prot_domain_create();
    KTEST_ASSERT(domain != NULL, "Domain create failed");

    int ret = prot_domain_map_region(domain, 0x10000000, 0x80000000, 4096, 0);
    KTEST_ASSERT(ret == -1, "Mapping on PROT_MODE_NONE should always fail explicitly with ERR_NOT_SUPPORTED");

    prot_domain_destroy(domain);
    return true;
}

static ktest_case_t prot_domain_tests[] = {
    {"Protection Domain Create/Destroy", test_prot_domain_create_destroy},
    {"MPU Region Exhaustion", test_mpu_exhaustion},
    {"MPU Region Overlap Rejection", test_mpu_overlap},
    {"MMU Sparse Mapping", test_mmu_sparse_mapping},
    {"PROT_NONE Explicit Failing", test_prot_none_fail_fast}
};

void ktest_prot_domain_run(void) {
    ktest_run_suite("Protection Domain Unit Tests", prot_domain_tests, 5);
}

static int boot_test_prot_domain(void) {
  if (test_prot_domain_create_destroy() &&
      test_mpu_exhaustion() &&
      test_mpu_overlap() &&
      test_mmu_sparse_mapping() &&
      test_prot_none_fail_fast()) {
    return 0; // success
  }
  return -1;
}

REGISTER_BOOT_SELFTEST("prot_domain_basic", "memory", boot_test_prot_domain, BOOT_TEST_STAGE_MEMORY, BOOT_TEST_MANDATORY, 0, true)
