#ifndef BHARAT_SYSTEM_SHELL_H
#define BHARAT_SYSTEM_SHELL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SHELL_MAX_INPUT_LEN 256U
#define SHELL_MAX_TOKENS 16U
#define SHELL_MAX_OUTPUT_LEN 1024U

typedef enum {
    SHELL_RC_OK = 0,
    SHELL_RC_INVALID_ARG = 1,
    SHELL_RC_UNKNOWN_COMMAND = 2,
    SHELL_RC_FORBIDDEN = 3,
    SHELL_RC_BACKEND_UNAVAILABLE = 4,
    SHELL_RC_TIMEOUT = 5,
    SHELL_RC_PARSE_ERROR = 6,
    SHELL_RC_AUTH_LOCKED = 7,
    SHELL_RC_INTERNAL = 255
} shell_status_code_t;

typedef enum {
    SHELL_OUTPUT_TEXT = 0,
    SHELL_OUTPUT_KV = 1
} shell_output_mode_t;

typedef enum {
    SHELL_MODE_DEV = 0,
    SHELL_MODE_PROD = 1,
    SHELL_MODE_FACTORY = 2,
    SHELL_MODE_RECOVERY = 3
} shell_mode_t;

typedef enum {
    SHELL_CAP_NONE = 0,
    SHELL_CAP_SVC_WRITE = (1u << 0),
    SHELL_CAP_REBOOT = (1u << 1),
    SHELL_CAP_DIAG = (1u << 2),
    SHELL_CAP_FACTORY = (1u << 3)
} shell_cap_t;

typedef struct {
    shell_status_code_t code;
    const char* message;
    const char* payload;
} shell_response_t;

typedef struct {
    char* tokens[SHELL_MAX_TOKENS];
    size_t count;
} shell_argv_t;

typedef struct {
    shell_mode_t mode;
    uint32_t caps_mask;
    shell_output_mode_t output_mode;
    uint32_t failed_auth_count;
    bool locked;
} shell_session_t;

#endif
