#include "shell_registry.h"

#include <stdio.h>
#include <string.h>

static shell_response_t mk(shell_status_code_t code, const char* msg, const char* payload) {
    shell_response_t r = {.code = code, .message = msg, .payload = payload};
    return r;
}

static shell_response_t cmd_help(const shell_session_t* session,
                                 const shell_backend_api_t* backend,
                                 const shell_argv_t* argv) {
    (void)session; (void)backend; (void)argv;
    return mk(SHELL_RC_OK,
              "commands",
              "help version uptime echo status sys info svc list svc status log tail health summary dev list mem stat reboot");
}

static shell_response_t cmd_version(const shell_session_t* s, const shell_backend_api_t* b, const shell_argv_t* a) {
    (void)s; (void)b; (void)a;
    return mk(SHELL_RC_OK, "version", "bharat-shell/1.0");
}

static shell_response_t cmd_uptime(const shell_session_t* s, const shell_backend_api_t* b, const shell_argv_t* a) {
    static char payload[64];
    uint64_t ms = 0;
    (void)s; (void)a;
    if (!b || !b->get_uptime_ms || b->get_uptime_ms(&ms) != 0) {
        return mk(SHELL_RC_BACKEND_UNAVAILABLE, "uptime unavailable", NULL);
    }
    snprintf(payload, sizeof(payload), "uptime_ms=%llu", (unsigned long long)ms);
    return mk(SHELL_RC_OK, "uptime", payload);
}

static shell_response_t cmd_echo(const shell_session_t* s, const shell_backend_api_t* b, const shell_argv_t* a) {
    static char payload[SHELL_MAX_OUTPUT_LEN];
    size_t i;
    size_t used = 0;
    (void)s; (void)b;
    payload[0] = '\0';
    for (i = 1; i < a->count; ++i) {
        int n = snprintf(payload + used, sizeof(payload) - used, "%s%s", (i > 1) ? " " : "", a->tokens[i]);
        if (n < 0 || (size_t)n >= sizeof(payload) - used) {
            break;
        }
        used += (size_t)n;
    }
    return mk(SHELL_RC_OK, "echo", payload);
}

static shell_response_t cmd_status(const shell_session_t* s, const shell_backend_api_t* b, const shell_argv_t* a) {
    static char payload[SHELL_MAX_OUTPUT_LEN];
    (void)s; (void)a;
    if (!b || !b->get_status || b->get_status(payload, sizeof(payload)) != 0) {
        return mk(SHELL_RC_BACKEND_UNAVAILABLE, "status unavailable", NULL);
    }
    return mk(SHELL_RC_OK, "status", payload);
}

static shell_response_t cmd_sys_info(const shell_session_t* s, const shell_backend_api_t* b, const shell_argv_t* a) {
    static char payload[SHELL_MAX_OUTPUT_LEN];
    (void)s; (void)a;
    if (!b || !b->get_sys_info || b->get_sys_info(payload, sizeof(payload)) != 0) {
        return mk(SHELL_RC_BACKEND_UNAVAILABLE, "sys info unavailable", NULL);
    }
    return mk(SHELL_RC_OK, "sys info", payload);
}

static shell_response_t cmd_svc_list(const shell_session_t* s, const shell_backend_api_t* b, const shell_argv_t* a) {
    static char payload[SHELL_MAX_OUTPUT_LEN];
    (void)s; (void)a;
    if (!b || !b->svc_list || b->svc_list(payload, sizeof(payload)) != 0) {
        return mk(SHELL_RC_BACKEND_UNAVAILABLE, "svc list unavailable", NULL);
    }
    return mk(SHELL_RC_OK, "svc list", payload);
}

static shell_response_t cmd_svc_status(const shell_session_t* s, const shell_backend_api_t* b, const shell_argv_t* a) {
    static char payload[SHELL_MAX_OUTPUT_LEN];
    (void)s;
    if (a->count < 3) {
        return mk(SHELL_RC_INVALID_ARG, "usage", "svc status <name>");
    }
    if (!b || !b->svc_status || b->svc_status(a->tokens[2], payload, sizeof(payload)) != 0) {
        return mk(SHELL_RC_BACKEND_UNAVAILABLE, "svc status unavailable", NULL);
    }
    return mk(SHELL_RC_OK, "svc status", payload);
}

static shell_response_t cmd_log_tail(const shell_session_t* s, const shell_backend_api_t* b, const shell_argv_t* a) {
    static char payload[SHELL_MAX_OUTPUT_LEN];
    (void)s; (void)a;
    if (!b || !b->log_tail || b->log_tail(payload, sizeof(payload)) != 0) {
        return mk(SHELL_RC_BACKEND_UNAVAILABLE, "log tail unavailable", NULL);
    }
    return mk(SHELL_RC_OK, "log tail", payload);
}

static shell_response_t cmd_health_summary(const shell_session_t* s, const shell_backend_api_t* b, const shell_argv_t* a) {
    static char payload[SHELL_MAX_OUTPUT_LEN];
    (void)s; (void)a;
    if (!b || !b->health_summary || b->health_summary(payload, sizeof(payload)) != 0) {
        return mk(SHELL_RC_BACKEND_UNAVAILABLE, "health unavailable", NULL);
    }
    return mk(SHELL_RC_OK, "health summary", payload);
}

static shell_response_t cmd_dev_list(const shell_session_t* s, const shell_backend_api_t* b, const shell_argv_t* a) {
    static char payload[SHELL_MAX_OUTPUT_LEN];
    (void)s; (void)a;
    if (!b || !b->dev_list || b->dev_list(payload, sizeof(payload)) != 0) {
        return mk(SHELL_RC_BACKEND_UNAVAILABLE, "dev list unavailable", NULL);
    }
    return mk(SHELL_RC_OK, "dev list", payload);
}

static shell_response_t cmd_mem_stat(const shell_session_t* s, const shell_backend_api_t* b, const shell_argv_t* a) {
    static char payload[SHELL_MAX_OUTPUT_LEN];
    (void)s; (void)a;
    if (!b || !b->mem_stat || b->mem_stat(payload, sizeof(payload)) != 0) {
        return mk(SHELL_RC_BACKEND_UNAVAILABLE, "mem stat unavailable", NULL);
    }
    return mk(SHELL_RC_OK, "mem stat", payload);
}


static shell_response_t cmd_diag_run(const shell_session_t* s, const shell_backend_api_t* b, const shell_argv_t* a) {
    volatile unsigned long i;
    (void)s; (void)b; (void)a;
    for (i = 0; i < 1000000ul; ++i) { }
    return mk(SHELL_RC_OK, "diag run", "complete");
}

static shell_response_t cmd_reboot(const shell_session_t* s, const shell_backend_api_t* b, const shell_argv_t* a) {
    (void)s; (void)a;
    if (!b || !b->reboot || b->reboot() != 0) {
        return mk(SHELL_RC_BACKEND_UNAVAILABLE, "reboot unavailable", NULL);
    }
    return mk(SHELL_RC_OK, "reboot", "accepted");
}

const shell_command_entry_t* shell_registry_get(size_t* count) {
    static const shell_command_entry_t entries[] = {
        {.command = "help", .required_caps = SHELL_CAP_NONE, .allowed_in_prod = true, .timeout_ms = 10, .handler = cmd_help},
        {.command = "version", .required_caps = SHELL_CAP_NONE, .allowed_in_prod = true, .timeout_ms = 10, .handler = cmd_version},
        {.command = "uptime", .required_caps = SHELL_CAP_NONE, .allowed_in_prod = true, .timeout_ms = 50, .handler = cmd_uptime},
        {.command = "echo", .required_caps = SHELL_CAP_NONE, .allowed_in_prod = true, .timeout_ms = 10, .handler = cmd_echo},
        {.command = "status", .required_caps = SHELL_CAP_NONE, .allowed_in_prod = true, .timeout_ms = 100, .handler = cmd_status},
        {.command = "sys info", .required_caps = SHELL_CAP_NONE, .allowed_in_prod = true, .timeout_ms = 100, .handler = cmd_sys_info},
        {.command = "svc list", .required_caps = SHELL_CAP_NONE, .allowed_in_prod = true, .timeout_ms = 100, .handler = cmd_svc_list},
        {.command = "svc status", .required_caps = SHELL_CAP_NONE, .allowed_in_prod = true, .timeout_ms = 100, .handler = cmd_svc_status},
        {.command = "log tail", .required_caps = SHELL_CAP_NONE, .allowed_in_prod = true, .timeout_ms = 100, .handler = cmd_log_tail},
        {.command = "health summary", .required_caps = SHELL_CAP_NONE, .allowed_in_prod = true, .timeout_ms = 100, .handler = cmd_health_summary},
        {.command = "dev list", .required_caps = SHELL_CAP_NONE, .allowed_in_prod = true, .timeout_ms = 100, .handler = cmd_dev_list},
        {.command = "mem stat", .required_caps = SHELL_CAP_NONE, .allowed_in_prod = true, .timeout_ms = 100, .handler = cmd_mem_stat},
        {.command = "diag run", .required_caps = SHELL_CAP_DIAG, .allowed_in_prod = false, .timeout_ms = 1, .handler = cmd_diag_run},
        {.command = "reboot", .required_caps = SHELL_CAP_REBOOT, .allowed_in_prod = false, .timeout_ms = 200, .handler = cmd_reboot},
    };
    if (count) {
        *count = sizeof(entries) / sizeof(entries[0]);
    }
    return entries;
}
