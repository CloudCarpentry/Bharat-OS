#include "shell_backend.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

static int write_text(char* out, size_t out_len, const char* value) {
    if (!out || out_len == 0u || !value) {
        return -1;
    }
    if (snprintf(out, out_len, "%s", value) < 0) {
        return -1;
    }
    return 0;
}

static int backend_uptime(uint64_t* uptime_ms) {
    if (!uptime_ms) {
        return -1;
    }
    *uptime_ms = (uint64_t)((clock() * 1000ull) / (uint64_t)CLOCKS_PER_SEC);
    return 0;
}

static int backend_status(char* out, size_t out_len) {
    return write_text(out, out_len, "state=running mode=normal");
}

static int backend_sys_info(char* out, size_t out_len) {
    return write_text(out, out_len, "os=Bharat-OS shell=min-system profile=admin");
}

static int backend_svc_list(char* out, size_t out_len) {
    return write_text(out, out_len, "console diag filesystem");
}

static int backend_svc_status(const char* name, char* out, size_t out_len) {
    if (!name || name[0] == '\0') {
        return -1;
    }
    if (snprintf(out, out_len, "service=%s status=active", name) < 0) {
        return -1;
    }
    return 0;
}

static int backend_log_tail(char* out, size_t out_len) {
    return write_text(out, out_len, "log[tail]=no-errors");
}

static int backend_health(char* out, size_t out_len) {
    return write_text(out, out_len, "health=ok checks=all");
}

static int backend_dev_list(char* out, size_t out_len) {
    return write_text(out, out_len, "uart0 rtc0 net0");
}

static int backend_mem_stat(char* out, size_t out_len) {
    return write_text(out, out_len, "mem_total_kb=65536 mem_free_kb=32768");
}

static int backend_reboot(void) {
    return 0;
}

static void backend_audit(const char* event, const char* command, shell_status_code_t status) {
    (void)fprintf(stderr,
                  "shell_audit event=%s command=%s status=%u\n",
                  event ? event : "none",
                  command ? command : "none",
                  (unsigned)status);
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
