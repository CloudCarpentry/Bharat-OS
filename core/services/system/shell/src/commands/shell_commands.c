#include "shell_registry.h"

#include "shell_string.h"

static shell_response_t mk(shell_status_code_t code, const char* msg, const char* payload);

static shell_response_t cmd_diag_query(const shell_backend_api_t* backend, bh_shell_diag_query_t query,
                                       const char* message, const shell_argv_t* argv, size_t argument_index) {
    static char payload[SHELL_MAX_OUTPUT_LEN];
    bh_shell_diag_request_v1_t request = {0};
    size_t argument_len = 0u;
    if (!backend || !backend->diag_query) return mk(SHELL_RC_BACKEND_UNAVAILABLE, "diagnostic service unavailable", NULL);
    if ((query == BH_SHELL_DIAG_SERVICE_STATUS || query == BH_SHELL_DIAG_HMEM_ALLOC) && argument_index >= argv->count)
        return mk(SHELL_RC_INVALID_ARG, "missing argument", NULL);
    if (argument_index < argv->count) {
        argument_len = shell_strlen(argv->tokens[argument_index]);
        if (argument_len >= sizeof(request.argument)) return mk(SHELL_RC_INVALID_ARG, "argument too long", NULL);
        shell_memcpy(request.argument, argv->tokens[argument_index], argument_len + 1u);
    }
    request.abi_version = BH_SHELL_DIAG_ABI_VERSION;
    request.struct_size = (uint16_t)sizeof(request);
    request.query = (uint32_t)query;
    request.argument_length = (uint32_t)argument_len;
    if (backend->diag_query(&request, payload, sizeof(payload)) != 0)
        return mk(SHELL_RC_BACKEND_UNAVAILABLE, "diagnostic query unavailable", NULL);
    return mk(SHELL_RC_OK, message, payload);
}

static void write_u64_dec(char* out, size_t out_len, uint64_t value) {
    char tmp[21];
    size_t n = 0;
    size_t i;
    if (!out || out_len == 0u) {
        return;
    }
    do {
        tmp[n++] = (char)('0' + (value % 10u));
        value /= 10u;
    } while (value != 0u && n < sizeof(tmp));
    i = 0;
    while (n > 0 && (i + 1u) < out_len) {
        out[i++] = tmp[--n];
    }
    out[i] = '\0';
}

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
              "help version uptime cpuinfo hwcap ps service list service status meminfo vmstat hmem info hmem topology hmem stats hmem alloc hmem bench tensor info tensor bench timer info timer test capstat device list io stat net status diag");
}

static shell_response_t cmd_version(const shell_session_t* s, const shell_backend_api_t* b, const shell_argv_t* a) {
    (void)s; (void)b; (void)a;
    return mk(SHELL_RC_OK, "version", "bharat-shell/1.0");
}

static shell_response_t cmd_uptime(const shell_session_t* s, const shell_backend_api_t* b, const shell_argv_t* a) {
    static char payload[64];
    static const char prefix[] = "uptime_ms=";
    char digits[32];
    uint64_t ms = 0;
    (void)s; (void)a;
    if (!b || !b->get_uptime_ms || b->get_uptime_ms(&ms) != 0) {
        return mk(SHELL_RC_BACKEND_UNAVAILABLE, "uptime unavailable", NULL);
    }
    write_u64_dec(digits, sizeof(digits), ms);
    shell_memcpy(payload, prefix, sizeof(prefix) - 1u);
    payload[sizeof(prefix) - 1u] = '\0';
    {
        size_t pfx_len = sizeof(prefix) - 1u;
        size_t digits_len = shell_strlen(digits);
        if ((pfx_len + digits_len) >= sizeof(payload)) {
            digits_len = sizeof(payload) - pfx_len - 1u;
        }
        shell_memcpy(payload + pfx_len, digits, digits_len);
        payload[pfx_len + digits_len] = '\0';
    }
    return mk(SHELL_RC_OK, "uptime", payload);
}

static shell_response_t cmd_echo(const shell_session_t* s, const shell_backend_api_t* b, const shell_argv_t* a) {
    static char payload[SHELL_MAX_OUTPUT_LEN];
    size_t i;
    size_t used = 0;
    (void)s; (void)b;
    payload[0] = '\0';
    for (i = 1; i < a->count; ++i) {
        const char* token = a->tokens[i];
        size_t token_len = shell_strlen(token);
        size_t add = token_len + ((i > 1) ? 1u : 0u);
        if (add >= (sizeof(payload) - used)) {
            break;
        }
        if (i > 1) {
            payload[used++] = ' ';
        }
        shell_memcpy(payload + used, token, token_len);
        used += token_len;
        payload[used] = '\0';
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
    static char payload[SHELL_MAX_OUTPUT_LEN];
    (void)s; (void)a;
    if (!b || !b->diag_run || b->diag_run(payload, sizeof(payload)) != 0) {
        return mk(SHELL_RC_BACKEND_UNAVAILABLE, "diagnostics unavailable", NULL);
    }
    return mk(SHELL_RC_OK, "diag run", payload);
}

static shell_response_t cmd_reboot(const shell_session_t* s, const shell_backend_api_t* b, const shell_argv_t* a) {
    (void)s; (void)a;
    if (!b || !b->reboot || b->reboot() != 0) {
        return mk(SHELL_RC_BACKEND_UNAVAILABLE, "reboot unavailable", NULL);
    }
    return mk(SHELL_RC_OK, "reboot", "accepted");
}

#define DIAG_HANDLER(name, query, label, arg_index) \
static shell_response_t name(const shell_session_t* s, const shell_backend_api_t* b, const shell_argv_t* a) { \
    (void)s; return cmd_diag_query(b, query, label, a, arg_index); \
}
DIAG_HANDLER(cmd_cpuinfo, BH_SHELL_DIAG_CPU_INFO, "cpuinfo", 1u)
DIAG_HANDLER(cmd_hwcap, BH_SHELL_DIAG_HW_CAP, "hwcap", 1u)
DIAG_HANDLER(cmd_ps, BH_SHELL_DIAG_PROCESS_LIST, "ps", 1u)
DIAG_HANDLER(cmd_service_list, BH_SHELL_DIAG_SERVICE_LIST, "service list", 2u)
DIAG_HANDLER(cmd_service_status, BH_SHELL_DIAG_SERVICE_STATUS, "service status", 2u)
DIAG_HANDLER(cmd_meminfo, BH_SHELL_DIAG_MEMORY_INFO, "meminfo", 1u)
DIAG_HANDLER(cmd_vmstat, BH_SHELL_DIAG_VM_STAT, "vmstat", 1u)
DIAG_HANDLER(cmd_hmem_info, BH_SHELL_DIAG_HMEM_INFO, "hmem info", 2u)
DIAG_HANDLER(cmd_hmem_topology, BH_SHELL_DIAG_HMEM_TOPOLOGY, "hmem topology", 2u)
DIAG_HANDLER(cmd_hmem_stats, BH_SHELL_DIAG_HMEM_STATS, "hmem stats", 2u)
DIAG_HANDLER(cmd_hmem_alloc, BH_SHELL_DIAG_HMEM_ALLOC, "hmem alloc", 2u)
DIAG_HANDLER(cmd_hmem_bench, BH_SHELL_DIAG_HMEM_BENCH, "hmem bench", 2u)
DIAG_HANDLER(cmd_tensor_info, BH_SHELL_DIAG_TENSOR_INFO, "tensor info", 2u)
DIAG_HANDLER(cmd_tensor_bench, BH_SHELL_DIAG_TENSOR_BENCH, "tensor bench", 2u)
DIAG_HANDLER(cmd_timer_info, BH_SHELL_DIAG_TIMER_INFO, "timer info", 2u)
DIAG_HANDLER(cmd_timer_test, BH_SHELL_DIAG_TIMER_TEST, "timer test", 2u)
DIAG_HANDLER(cmd_capstat, BH_SHELL_DIAG_CAP_STAT, "capstat", 1u)
DIAG_HANDLER(cmd_device_list, BH_SHELL_DIAG_DEVICE_LIST, "device list", 2u)
DIAG_HANDLER(cmd_io_stat, BH_SHELL_DIAG_IO_STAT, "io stat", 2u)
DIAG_HANDLER(cmd_net_status, BH_SHELL_DIAG_NET_STATUS, "net status", 2u)
DIAG_HANDLER(cmd_diag, BH_SHELL_DIAG_RUN, "diag", 1u)

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
        {.command = "cpuinfo", .required_caps = SHELL_CAP_NONE, .allowed_in_prod = true, .timeout_ms = 100, .handler = cmd_cpuinfo},
        {.command = "hwcap", .required_caps = SHELL_CAP_NONE, .allowed_in_prod = true, .timeout_ms = 100, .handler = cmd_hwcap},
        {.command = "ps", .required_caps = SHELL_CAP_NONE, .allowed_in_prod = true, .timeout_ms = 100, .handler = cmd_ps},
        {.command = "service list", .required_caps = SHELL_CAP_NONE, .allowed_in_prod = true, .timeout_ms = 100, .handler = cmd_service_list},
        {.command = "service status", .required_caps = SHELL_CAP_NONE, .allowed_in_prod = true, .timeout_ms = 100, .handler = cmd_service_status},
        {.command = "meminfo", .required_caps = SHELL_CAP_NONE, .allowed_in_prod = true, .timeout_ms = 100, .handler = cmd_meminfo},
        {.command = "vmstat", .required_caps = SHELL_CAP_NONE, .allowed_in_prod = true, .timeout_ms = 100, .handler = cmd_vmstat},
        {.command = "hmem info", .required_caps = SHELL_CAP_NONE, .allowed_in_prod = true, .timeout_ms = 100, .handler = cmd_hmem_info},
        {.command = "hmem topology", .required_caps = SHELL_CAP_NONE, .allowed_in_prod = true, .timeout_ms = 100, .handler = cmd_hmem_topology},
        {.command = "hmem stats", .required_caps = SHELL_CAP_NONE, .allowed_in_prod = true, .timeout_ms = 100, .handler = cmd_hmem_stats},
        {.command = "hmem alloc", .required_caps = SHELL_CAP_DIAG, .allowed_in_prod = false, .timeout_ms = 200, .handler = cmd_hmem_alloc},
        {.command = "hmem bench", .required_caps = SHELL_CAP_DIAG, .allowed_in_prod = false, .timeout_ms = 1000, .handler = cmd_hmem_bench},
        {.command = "tensor info", .required_caps = SHELL_CAP_NONE, .allowed_in_prod = true, .timeout_ms = 100, .handler = cmd_tensor_info},
        {.command = "tensor bench", .required_caps = SHELL_CAP_DIAG, .allowed_in_prod = false, .timeout_ms = 1000, .handler = cmd_tensor_bench},
        {.command = "timer info", .required_caps = SHELL_CAP_NONE, .allowed_in_prod = true, .timeout_ms = 100, .handler = cmd_timer_info},
        {.command = "timer test", .required_caps = SHELL_CAP_DIAG, .allowed_in_prod = false, .timeout_ms = 2000, .handler = cmd_timer_test},
        {.command = "capstat", .required_caps = SHELL_CAP_NONE, .allowed_in_prod = true, .timeout_ms = 100, .handler = cmd_capstat},
        {.command = "device list", .required_caps = SHELL_CAP_NONE, .allowed_in_prod = true, .timeout_ms = 100, .handler = cmd_device_list},
        {.command = "io stat", .required_caps = SHELL_CAP_NONE, .allowed_in_prod = true, .timeout_ms = 100, .handler = cmd_io_stat},
        {.command = "net status", .required_caps = SHELL_CAP_NONE, .allowed_in_prod = true, .timeout_ms = 100, .handler = cmd_net_status},
        {.command = "diag", .required_caps = SHELL_CAP_DIAG, .allowed_in_prod = false, .timeout_ms = 1000, .handler = cmd_diag},
    };
    if (count) {
        *count = sizeof(entries) / sizeof(entries[0]);
    }
    return entries;
}
