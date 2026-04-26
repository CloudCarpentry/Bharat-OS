#include "shell_backend.h"

#include "shell_string.h"
#include "urpc/urpc.h"

#include <stdbool.h>
#include <stdint.h>

#define SHELL_URPC_RING_SLOTS 8u
#define SHELL_URPC_TIMEOUT_SPINS 1024u

typedef enum {
    SHELL_URPC_OP_UPTIME = 1,
    SHELL_URPC_OP_STATUS = 2,
    SHELL_URPC_OP_SYS_INFO = 3,
    SHELL_URPC_OP_SVC_LIST = 4,
    SHELL_URPC_OP_SVC_STATUS = 5,
    SHELL_URPC_OP_LOG_TAIL = 6,
    SHELL_URPC_OP_HEALTH_SUMMARY = 7,
    SHELL_URPC_OP_DEV_LIST = 8,
    SHELL_URPC_OP_MEM_STAT = 9,
    SHELL_URPC_OP_REBOOT = 10
} shell_urpc_op_t;

typedef struct {
    bool initialized;
    urpc_channel_t req;
    urpc_channel_t resp;
    urpc_msg_t req_storage[SHELL_URPC_RING_SLOTS];
    urpc_msg_t resp_storage[SHELL_URPC_RING_SLOTS];
    uint64_t fake_uptime_ms;
} shell_urpc_ctx_t;

static shell_urpc_ctx_t g_ctx;

static void shell_backend_urpc_init(void) {
    if (g_ctx.initialized) {
        return;
    }

    (void)urpc_init_channel(&g_ctx.req, g_ctx.req_storage, sizeof(g_ctx.req_storage));
    (void)urpc_init_channel(&g_ctx.resp, g_ctx.resp_storage, sizeof(g_ctx.resp_storage));
    g_ctx.fake_uptime_ms = 0;
    g_ctx.initialized = true;
}

static int make_service_response(const urpc_msg_t* req, urpc_msg_t* resp) {
    const char* fixed = NULL;
    const char* name;
    size_t name_len;

    if (!req || !resp) {
        return -1;
    }

    resp->type = req->type;
    resp->length = 0;

    switch ((shell_urpc_op_t)req->type) {
        case SHELL_URPC_OP_UPTIME: {
            uint64_t ms;
            g_ctx.fake_uptime_ms += 100u;
            ms = g_ctx.fake_uptime_ms;
            shell_memcpy(resp->payload, &ms, sizeof(ms));
            resp->length = (uint32_t)sizeof(ms);
            return 0;
        }
        case SHELL_URPC_OP_STATUS:
            fixed = "state=running mode=normal transport=urpc";
            break;
        case SHELL_URPC_OP_SYS_INFO:
            fixed = "os=Bharat-OS shell=system profile=admin transport=urpc";
            break;
        case SHELL_URPC_OP_SVC_LIST:
            fixed = "console shell diag filesystem";
            break;
        case SHELL_URPC_OP_LOG_TAIL:
            fixed = "log[tail]=no-errors";
            break;
        case SHELL_URPC_OP_HEALTH_SUMMARY:
            fixed = "health=ok checks=all";
            break;
        case SHELL_URPC_OP_DEV_LIST:
            fixed = "uart0 rtc0 net0";
            break;
        case SHELL_URPC_OP_MEM_STAT:
            fixed = "mem_total_kb=65536 mem_free_kb=32768";
            break;
        case SHELL_URPC_OP_REBOOT:
            fixed = "accepted";
            break;
        case SHELL_URPC_OP_SVC_STATUS:
            name = (const char*)req->payload;
            name_len = shell_strlen(name);
            if (name_len == 0u) {
                return -1;
            }
            if ((sizeof("service=") - 1u) + name_len + (sizeof(" status=active") - 1u) >= sizeof(resp->payload)) {
                return -1;
            }
            shell_memcpy(resp->payload, "service=", sizeof("service=") - 1u);
            shell_memcpy(resp->payload + (sizeof("service=") - 1u), name, name_len);
            shell_memcpy(resp->payload + (sizeof("service=") - 1u) + name_len,
                         " status=active",
                         sizeof(" status=active"));
            resp->length = (uint32_t)((sizeof("service=") - 1u) + name_len + sizeof(" status=active"));
            return 0;
        default:
            return -1;
    }

    if (!fixed) {
        return -1;
    }

    resp->length = (uint32_t)(shell_strlen(fixed) + 1u);
    if (resp->length > sizeof(resp->payload)) {
        return -1;
    }

    shell_memcpy(resp->payload, fixed, resp->length);
    return 0;
}

static int exchange(shell_urpc_op_t op, const char* arg, char* out, size_t out_len, uint64_t* out_u64) {
    urpc_msg_t req;
    urpc_msg_t svc_req;
    urpc_msg_t svc_resp;
    urpc_msg_t resp;
    uint32_t spins;

    shell_backend_urpc_init();

    req.type = (uint32_t)op;
    req.length = 0;
    if (arg) {
        size_t arg_len = shell_strlen(arg) + 1u;
        if (arg_len > sizeof(req.payload)) {
            return -1;
        }
        shell_memcpy(req.payload, arg, arg_len);
        req.length = (uint32_t)arg_len;
    }

    if (urpc_send(&g_ctx.req, &req) != URPC_SUCCESS) {
        return -1;
    }

    if (urpc_receive(&g_ctx.req, &svc_req) != URPC_SUCCESS) {
        return -1;
    }

    if (make_service_response(&svc_req, &svc_resp) != 0) {
        return -1;
    }

    if (urpc_send(&g_ctx.resp, &svc_resp) != URPC_SUCCESS) {
        return -1;
    }

    spins = 0;
    while (urpc_receive(&g_ctx.resp, &resp) != URPC_SUCCESS) {
        ++spins;
        if (spins >= SHELL_URPC_TIMEOUT_SPINS) {
            return -1;
        }
    }

    if (out_u64) {
        if (resp.length < sizeof(uint64_t)) {
            return -1;
        }
        shell_memcpy(out_u64, resp.payload, sizeof(uint64_t));
        return 0;
    }

    if (!out || out_len == 0u || resp.length == 0u || resp.length > sizeof(resp.payload) || resp.length > out_len) {
        return -1;
    }

    shell_memcpy(out, resp.payload, resp.length);
    out[out_len - 1u] = '\0';
    return 0;
}

static int backend_uptime(uint64_t* uptime_ms) {
    return exchange(SHELL_URPC_OP_UPTIME, NULL, NULL, 0u, uptime_ms);
}

static int backend_status(char* out, size_t out_len) {
    return exchange(SHELL_URPC_OP_STATUS, NULL, out, out_len, NULL);
}

static int backend_sys_info(char* out, size_t out_len) {
    return exchange(SHELL_URPC_OP_SYS_INFO, NULL, out, out_len, NULL);
}

static int backend_svc_list(char* out, size_t out_len) {
    return exchange(SHELL_URPC_OP_SVC_LIST, NULL, out, out_len, NULL);
}

static int backend_svc_status(const char* name, char* out, size_t out_len) {
    if (!name || name[0] == '\0') {
        return -1;
    }
    return exchange(SHELL_URPC_OP_SVC_STATUS, name, out, out_len, NULL);
}

static int backend_log_tail(char* out, size_t out_len) {
    return exchange(SHELL_URPC_OP_LOG_TAIL, NULL, out, out_len, NULL);
}

static int backend_health(char* out, size_t out_len) {
    return exchange(SHELL_URPC_OP_HEALTH_SUMMARY, NULL, out, out_len, NULL);
}

static int backend_dev_list(char* out, size_t out_len) {
    return exchange(SHELL_URPC_OP_DEV_LIST, NULL, out, out_len, NULL);
}

static int backend_mem_stat(char* out, size_t out_len) {
    return exchange(SHELL_URPC_OP_MEM_STAT, NULL, out, out_len, NULL);
}

static int backend_reboot(void) {
    char ack[16];

    if (exchange(SHELL_URPC_OP_REBOOT, NULL, ack, sizeof(ack), NULL) != 0) {
        return -1;
    }

    return (shell_strcmp(ack, "accepted") == 0) ? 0 : -1;
}

static void backend_audit(const char* event, const char* command, shell_status_code_t status) {
    (void)event;
    (void)command;
    (void)status;
}

const shell_backend_api_t* shell_default_backend(void) {
    static const shell_backend_api_t api = {
        .get_uptime_ms = backend_uptime,
        .get_status = backend_status,
        .get_sys_info = backend_sys_info,
        .svc_list = backend_svc_list,
        .svc_status = backend_svc_status,
        .log_tail = backend_log_tail,
        .health_summary = backend_health,
        .dev_list = backend_dev_list,
        .mem_stat = backend_mem_stat,
        .reboot = backend_reboot,
        .audit_event = backend_audit,
    };
    return &api;
}
