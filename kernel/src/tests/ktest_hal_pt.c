#include "../../include/tests/ktest.h"
#include "../../include/hal/hal_pt.h"
#include "../../include/hal/hal_tlb.h"
#include "../../include/mm.h"
#include "../../include/numa.h"
#include "bharat/console.h"
#define KPRINT(s) console_write_raw(s)

#include "../../include/kernel.h"
#include "../../include/bharat/boot_info.h"

void ktest_hal_pt_run(void) {
    KPRINT("Running HAL PT Conformance Tests...\n");

    if (!active_hal_pt) {
        KPRINT("FAIL: active_hal_pt is NULL\n");
        return;
    }

    // Test 1: Address Space Creation
    phys_addr_t new_as = active_hal_pt->create_address_space(0);
    if (new_as == 0) {
        KPRINT("FAIL: create_address_space returned 0\n");
        return;
    }

    // Test 2: Map Page
    virt_addr_t test_vaddr = 0x10000000;
    phys_addr_t test_paddr = mm_alloc_page(NUMA_NODE_ANY);
    if (test_paddr == 0) {
        KPRINT("FAIL: could not allocate physical page\n");
        return;
    }

    int map_res = active_hal_pt->map_page(new_as, test_vaddr, test_paddr, HAL_PT_FLAG_READ | HAL_PT_FLAG_WRITE | HAL_PT_FLAG_USER);
    if (map_res != 0) {
        KPRINT("FAIL: map_page failed\n");
        return;
    }

    // Test 3: Query Page
    phys_addr_t query_paddr = 0;
    uint32_t query_flags = 0;
    int query_res = active_hal_pt->query_page(new_as, test_vaddr, &query_paddr, &query_flags);
    if (query_res != 0) {
        KPRINT("FAIL: query_page failed\n");
        return;
    }
    if (query_paddr != test_paddr) {
        KPRINT("FAIL: query_page returned wrong physical address\n");
        return;
    }
    if (!(query_flags & HAL_PT_FLAG_READ) || !(query_flags & HAL_PT_FLAG_WRITE) || !(query_flags & HAL_PT_FLAG_USER)) {
        KPRINT("FAIL: query_page returned wrong flags\n");
        return;
    }

    // Test 4: Protect Page
    int protect_res = active_hal_pt->protect_page(new_as, test_vaddr, HAL_PT_FLAG_READ | HAL_PT_FLAG_USER);
    if (protect_res != 0) {
        KPRINT("FAIL: protect_page failed\n");
        return;
    }

    query_flags = 0;
    query_res = active_hal_pt->query_page(new_as, test_vaddr, NULL, &query_flags);
    if (query_res == 0) {
        if ((query_flags & HAL_PT_FLAG_WRITE) != 0) {
             KPRINT("FAIL: protect_page did not remove write flag\n");
             return;
        }
    }

    // Test 5: Unmap Page
    phys_addr_t unmapped_paddr = 0;
    int unmap_res = active_hal_pt->unmap_page(new_as, test_vaddr, &unmapped_paddr);
    if (unmap_res != 0) {
        KPRINT("FAIL: unmap_page failed\n");
        return;
    }
    if (unmapped_paddr != test_paddr) {
        KPRINT("FAIL: unmap_page returned wrong physical address\n");
        return;
    }

    // Test 6: Query unmapped page should fail
    query_res = active_hal_pt->query_page(new_as, test_vaddr, NULL, NULL);
    if (query_res == 0) {
        KPRINT("FAIL: query_page succeeded on unmapped page\n");
        return;
    }

    // Test 7: Address Space Destruction
    active_hal_pt->destroy_address_space(new_as);
    mm_free_page(test_paddr);

    // Test 8: Large Page Mapping (2MB)
    if (active_hal_pt->map_range) {
        phys_addr_t huge_as = active_hal_pt->create_address_space(0);
        if (huge_as != 0) {
            virt_addr_t huge_vaddr = 0x40000000;
            phys_addr_t huge_paddr = 0x80000000; // Mock aligned physical address

            int huge_res = active_hal_pt->map_range(huge_as, huge_vaddr, huge_paddr, 0x200000, HAL_PT_FLAG_READ | HAL_PT_FLAG_WRITE | HAL_PT_FLAG_LARGE_2M);
            if (huge_res == 0) {
                phys_addr_t q_pa = 0;
                uint32_t q_flags = 0;
                size_t q_size = 0;
                int q_res = active_hal_pt->query_mapping(huge_as, huge_vaddr, &q_pa, &q_size, &q_flags);

                if (q_res == 0 && q_size == 0x200000 && (q_flags & HAL_PT_FLAG_LARGE_2M)) {
                    KPRINT("PASS: Large Page 2MB Mapping\n");
                } else {
                    KPRINT("FAIL: Large Page 2MB query failed or returned wrong size/flags\n");
                }
            } else {
                KPRINT("FAIL: map_range for 2MB failed\n");
            }
            active_hal_pt->destroy_address_space(huge_as);
        }
    }

    KPRINT("PASS: HAL PT Conformance Tests\n");
    return;
}
