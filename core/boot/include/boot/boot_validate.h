#ifndef BHARAT_BOOT_VALIDATE_H
#define BHARAT_BOOT_VALIDATE_H

#include "boot_info.h"
#include "boot_errno.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int error_code;
    const char *message;
    uint32_t failing_index;
    bool is_fatal;
} boot_validation_report_t;

int boot_validate_basic(const boot_info_t *bi, boot_validation_report_t *report);
int boot_validate_memory_map(const boot_info_t *bi, boot_validation_report_t *report);
int boot_validate_modules(const boot_info_t *bi, boot_validation_report_t *report);
int boot_validate_console(const boot_info_t *bi, boot_validation_report_t *report);
int boot_validate_firmware(const boot_info_t *bi, boot_validation_report_t *report);
int boot_validate_security(const boot_info_t *bi, boot_validation_report_t *report);

int boot_validate_all(boot_info_t *bi, boot_validation_report_t *report);

#ifdef __cplusplus
}
#endif

#endif // BHARAT_BOOT_VALIDATE_H
