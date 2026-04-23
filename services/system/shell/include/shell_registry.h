#ifndef BHARAT_SYSTEM_SHELL_REGISTRY_H
#define BHARAT_SYSTEM_SHELL_REGISTRY_H

#include "shell.h"
#include "shell_backend.h"

typedef shell_response_t (*shell_handler_t)(const shell_session_t* session,
                                            const shell_backend_api_t* backend,
                                            const shell_argv_t* argv);

typedef struct {
    const char* command;
    uint32_t required_caps;
    bool allowed_in_prod;
    bool factory_only;
    bool recovery_only;
    uint32_t timeout_ms;
    shell_handler_t handler;
} shell_command_entry_t;

const shell_command_entry_t* shell_registry_get(size_t* count);

#endif
