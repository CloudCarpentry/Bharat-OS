#ifndef BHARAT_UAPI_SHELL_RIGHTS_H
#define BHARAT_UAPI_SHELL_RIGHTS_H

#include <stdint.h>

/**
 * Shell command rights for the granular service capability model.
 */
#define BHARAT_SHELL_RIGHT_NONE  0x00000000u
#define BHARAT_SHELL_RIGHT_DIAG  0x00000001u
#define BHARAT_SHELL_RIGHT_ADMIN 0x00000002u

typedef uint32_t bharat_shell_rights_t;

#endif /* BHARAT_UAPI_SHELL_RIGHTS_H */
