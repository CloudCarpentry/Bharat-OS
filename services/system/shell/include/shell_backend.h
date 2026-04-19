#ifndef BHARAT_SYSTEM_SHELL_BACKEND_H
#define BHARAT_SYSTEM_SHELL_BACKEND_H

#include <stddef.h>
#include "shell.h"

typedef struct {
    int (*get_uptime_ms)(uint64_t* uptime_ms);
    int (*get_status)(char* out, size_t out_len);
    int (*get_sys_info)(char* out, size_t out_len);
    int (*svc_list)(char* out, size_t out_len);
    int (*svc_status)(const char* name, char* out, size_t out_len);
    int (*log_tail)(char* out, size_t out_len);
    int (*health_summary)(char* out, size_t out_len);
    int (*dev_list)(char* out, size_t out_len);
    int (*mem_stat)(char* out, size_t out_len);
    int (*reboot)(void);
    void (*audit_event)(const char* event, const char* command, shell_status_code_t status);
} shell_backend_api_t;

const shell_backend_api_t* shell_default_backend(void);

#endif
