#include "boot/boot_info.h"
#include "boot/boot_validate.h"
#include "boot/boot_errno.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

void test_boot_info_init_and_validate_basic() {
    boot_info_t bi;
    boot_info_init(&bi);

    boot_validation_report_t report;
    int ret = boot_validate_basic(&bi, &report);
    assert(ret == BOOT_OK);
    assert(bi.magic == BHARAT_BOOT_INFO_MAGIC);

    bi.magic = 0;
    ret = boot_validate_basic(&bi, &report);
    assert(ret == BOOT_ERR_BAD_MAGIC);

    printf("Passed test_boot_info_init_and_validate_basic\n");
}

void test_memory_map_overlap() {
    boot_info_t bi;
    boot_info_init(&bi);

    boot_info_add_mem_region(&bi, 0x1000, 0x1000, BOOT_MEM_USABLE);
    boot_info_add_mem_region(&bi, 0x1800, 0x1000, BOOT_MEM_RESERVED);

    boot_validation_report_t report;
    int ret = boot_validate_memory_map(&bi, &report);
    assert(ret == BOOT_ERR_OVERLAPPING_MEM_RANGE);

    boot_info_init(&bi);
    boot_info_add_mem_region(&bi, 0x1000, 0x1000, BOOT_MEM_USABLE);
    boot_info_add_mem_region(&bi, 0x2000, 0x1000, BOOT_MEM_RESERVED);
    ret = boot_validate_memory_map(&bi, &report);
    assert(ret == BOOT_OK);

    printf("Passed test_memory_map_overlap\n");
}

void test_cmdline_bounds() {
    boot_info_t bi;
    boot_info_init(&bi);

    char long_cmd[2048];
    memset(long_cmd, 'A', sizeof(long_cmd));
    long_cmd[sizeof(long_cmd) - 1] = '\0';

    boot_info_set_cmdline(&bi, long_cmd, sizeof(long_cmd));

    boot_validation_report_t report;
    int ret = boot_validate_basic(&bi, &report);
    assert(ret == BOOT_OK); // It safely truncates

    assert(bi.cmdline[BHARAT_BOOT_CMDLINE_MAX_LEN - 1] == '\0');
    assert(bi.cmdline[BHARAT_BOOT_CMDLINE_MAX_LEN - 2] == 'A');

    printf("Passed test_cmdline_bounds\n");
}

int main() {
    test_boot_info_init_and_validate_basic();
    test_memory_map_overlap();
    test_cmdline_bounds();
    printf("All host boot validation tests passed.\n");
    return 0;
}
