#include "shell_dispatch.h"

#include "shell_string.h"

#include <sys/time.h>

#include "shell_auth.h"
#include "shell_registry.h"

static shell_response_t mk(shell_status_code_t code, const char* msg, const char* payload) {
    shell_response_t r = {.code = code, .message = msg, .payload = payload};
    return r;
}

static bool tokens_equal(const char* token, const char* command_token, size_t len) {
    size_t i;
    if (!token || !command_token) {
        return false;
    }
    for (i = 0; i < len; ++i) {
        if (token[i] != command_token[i]) {
            return false;
        }
    }
    return true;
}

static bool command_matches(const char* command, const shell_argv_t* argv) {
    size_t i = 0;
    size_t token_idx = 0;
    size_t token_start = 0;
    size_t token_len;

    if (!command || !argv) {
        return false;
    }

    while (command[i] != '\0') {
        if (token_idx >= argv->count) {
            return false;
        }
        token_start = i;
        while (command[i] != '\0' && command[i] != ' ') {
            ++i;
        }
        token_len = i - token_start;

        if (shell_strlen(argv->tokens[token_idx]) != token_len) {
            return false;
        }
        if (token_len > 0u &&
            !tokens_equal(argv->tokens[token_idx], &command[token_start], token_len)) {
            return false;
        }

        if (command[i] == ' ') {
            while (command[i] == ' ') {
                ++i;
            }
            token_idx++;
            continue;
        }
        break;
    }

    return true;
}

static size_t command_token_count(const char* command) {
    size_t count = 1;
    size_t i;
    for (i = 0; command[i] != '\0'; ++i) {
        if (command[i] == ' ' && command[i + 1] != '\0') {
            ++count;
        }
    }
    return count;
}

static uint64_t monotonic_ms(void) {
    struct timeval tv;

    if (gettimeofday(&tv, NULL) != 0) {
        return 0u;
    }

    return ((uint64_t)tv.tv_sec * 1000u) + ((uint64_t)tv.tv_usec / 1000u);
}

shell_response_t shell_dispatch(shell_session_t* session,
                                const shell_backend_api_t* backend,
                                const shell_argv_t* argv) {
    const shell_command_entry_t* entries;
    const shell_command_entry_t* matched = NULL;
    size_t entries_count = 0;
    size_t i;
    shell_status_code_t access;
    shell_response_t response;
    uint64_t start_ms;
    uint64_t end_ms;

    if (!session || !argv || argv->count == 0) {
        return mk(SHELL_RC_INVALID_ARG, "invalid input", NULL);
    }

    entries = shell_registry_get(&entries_count);
    for (i = 0; i < entries_count; ++i) {
        size_t needed = command_token_count(entries[i].command);
        if (argv->count < needed) {
            continue;
        }
        if (command_matches(entries[i].command, argv)) {
            matched = &entries[i];
            break;
        }
    }

    if (!matched) {
        return mk(SHELL_RC_UNKNOWN_COMMAND, "unknown command", NULL);
    }

    access = shell_check_command_access(session, matched);
    if (access != SHELL_RC_OK) {
        shell_session_auth_fail(session);
        if (backend && backend->audit_event) {
            backend->audit_event("denied", matched->command, access);
        }
        return mk(access, "access denied", matched->command);
    }

    start_ms = monotonic_ms();
    response = matched->handler(session, backend, argv);
    end_ms = monotonic_ms();

    if (matched->timeout_ms > 0u) {
        if (end_ms > start_ms && (end_ms - start_ms) > matched->timeout_ms) {
            if (backend && backend->audit_event) {
                backend->audit_event("timeout", matched->command, SHELL_RC_TIMEOUT);
            }
            return mk(SHELL_RC_TIMEOUT, "command timeout", matched->command);
        }
    }

    shell_session_auth_success(session);
    if (backend && backend->audit_event && response.code != SHELL_RC_OK) {
        backend->audit_event("failed", matched->command, response.code);
    }
    return response;
}
