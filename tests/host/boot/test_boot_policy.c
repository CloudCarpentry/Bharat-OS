#include "boot/boot_info.h"
#include "boot/boot_validate.h"
#include "boot/boot_errno.h"
#include "boot/boot_mode.h"
#include "boot/boot_security.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

void test_boot_mode_resolve() {
    boot_info_t bi;
    boot_info_init(&bi);
    boot_info_set_cmdline(&bi, "foo=bar mode=diagnostic console=uart", 0);

    bharat_boot_mode_t mode;
    boot_mode_resolve(&bi, &mode);
    assert(mode == BHARAT_BOOT_MODE_DIAGNOSTIC);

    boot_info_init(&bi);
    boot_info_set_cmdline(&bi, "foo=bar mode=recovery", 0);
    boot_mode_resolve(&bi, &mode);
    assert(mode == BHARAT_BOOT_MODE_RECOVERY);

    printf("Passed test_boot_mode_resolve\n");
}

void test_security_policy() {
    boot_info_t bi;
    boot_info_init(&bi);

    bi.selected_mode = BHARAT_BOOT_MODE_NORMAL;
    bi.security_info.secure_boot_present = false;
    bi.security_info.secure_boot_verified = true;

    boot_validation_report_t report;
    int ret = boot_validate_security(&bi, &report);
    assert(ret == BOOT_ERR_SECURITY_POLICY);

    bi.security_info.secure_boot_present = true;
    bi.security_info.secure_boot_verified = true;
    ret = boot_validate_security(&bi, &report);
    assert(ret == BOOT_OK);

    printf("Passed test_security_policy\n");
}

int main() {
    test_boot_mode_resolve();
    test_security_policy();
    printf("All host boot policy/security tests passed.\n");
    return 0;
}
