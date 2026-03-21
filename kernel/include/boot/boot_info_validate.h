#ifndef BHARAT_BOOT_INFO_VALIDATE_H
#define BHARAT_BOOT_INFO_VALIDATE_H

#include "bharat/boot_info.h"
#include <stdbool.h>

bool boot_info_validate(const boot_info_t *boot);
void boot_info_finalize(boot_info_t *boot);

#endif // BHARAT_BOOT_INFO_VALIDATE_H
