#ifndef BHARAT_SYSTEM_SHELL_AUTH_H
#define BHARAT_SYSTEM_SHELL_AUTH_H

#include "shell.h"
#include "shell_registry.h"

bool shell_session_is_locked(const shell_session_t* session);
void shell_session_auth_fail(shell_session_t* session);
void shell_session_auth_success(shell_session_t* session);
shell_status_code_t shell_check_command_access(const shell_session_t* session,
                                               const shell_command_entry_t* entry);

#endif
