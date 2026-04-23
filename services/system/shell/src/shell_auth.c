#include "shell_auth.h"

#define SHELL_AUTH_MAX_FAILURES 3u

bool shell_session_is_locked(const shell_session_t* session) {
    return session && session->locked;
}

void shell_session_auth_fail(shell_session_t* session) {
    if (!session) {
        return;
    }
    session->failed_auth_count++;
    if (session->failed_auth_count >= SHELL_AUTH_MAX_FAILURES) {
        session->locked = true;
    }
}

void shell_session_auth_success(shell_session_t* session) {
    if (!session) {
        return;
    }
    session->failed_auth_count = 0;
}

shell_status_code_t shell_check_command_access(const shell_session_t* session,
                                               const shell_command_entry_t* entry) {
    if (!session || !entry) {
        return SHELL_RC_INVALID_ARG;
    }
    if (session->locked) {
        return SHELL_RC_AUTH_LOCKED;
    }
    if ((entry->required_caps & session->caps_mask) != entry->required_caps) {
        return SHELL_RC_FORBIDDEN;
    }
    if (session->mode == SHELL_MODE_PROD && !entry->allowed_in_prod) {
        return SHELL_RC_FORBIDDEN;
    }
    if (entry->factory_only && session->mode != SHELL_MODE_FACTORY) {
        return SHELL_RC_FORBIDDEN;
    }
    if (entry->recovery_only && session->mode != SHELL_MODE_RECOVERY) {
        return SHELL_RC_FORBIDDEN;
    }
    return SHELL_RC_OK;
}
