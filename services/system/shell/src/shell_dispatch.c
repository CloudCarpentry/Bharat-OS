#include "shell_dispatch.h"

#include <string.h>
#include <time.h>

#include "shell_auth.h"
#include "shell_registry.h"

static shell_response_t mk(shell_status_code_t code, const char* msg, const char* payload) {
    shell_response_t r = {.code = code, .message = msg, .payload = payload};
    return r;
}

static bool command_matches(const char* command, const shell_argv_t* argv) {
    char buf[32];
    size_t i = 0;
    size_t token_idx = 0;
    size_t out = 0;

    if (!command || !argv) {
        return false;
    }

    while (command[i] != '\0' && token_idx < argv->count) {
        size_t token_len = strlen(argv->tokens[token_idx]);
        if (out + token_len + 1u >= sizeof(buf)) {
            return false;
        }
        memcpy(&buf[out], argv->tokens[token_idx], token_len);
        out += token_len;

        while (command[i] != '\0' && command[i] != ' ') {
            ++i;
        }
        if (command[i] == ' ') {
            buf[out++] = ' ';
            while (command[i] == ' ') {
                ++i;
            }
            token_idx++;
            continue;
        }
        break;
    }

    buf[out] = '\0';
    return strcmp(buf, command) == 0;
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

shell_response_t shell_dispatch(shell_session_t* session,
                                const shell_backend_api_t* backend,
                                const shell_argv_t* argv) {
    const shell_command_entry_t* entries;
    const shell_command_entry_t* matched = NULL;
    size_t entries_count = 0;
    size_t i;
    clock_t started;
    clock_t elapsed_ms;
    shell_status_code_t access;
    shell_response_t response;

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

    started = clock();
    response = matched->handler(session, backend, argv);
    elapsed_ms = ((clock() - started) * 1000) / CLOCKS_PER_SEC;
    if (matched->timeout_ms != 0u && (uint32_t)elapsed_ms > matched->timeout_ms) {
        if (backend && backend->audit_event) {
            backend->audit_event("timeout", matched->command, SHELL_RC_TIMEOUT);
        }
        return mk(SHELL_RC_TIMEOUT, "command timed out", matched->command);
    }

    shell_session_auth_success(session);
    if (backend && backend->audit_event && response.code != SHELL_RC_OK) {
        backend->audit_event("failed", matched->command, response.code);
    }
    return response;
}
