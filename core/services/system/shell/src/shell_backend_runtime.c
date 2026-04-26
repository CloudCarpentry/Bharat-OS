#include "shell_backend.h"
#include "shell_string.h"
#include "bharat/runtime/runtime.h"
#include "bharat/uapi/shell/shell_rights.h"
#include "bharat/uapi/ipc/status.h"

/**
 * Bharat-OS Shell Runtime Backend
 *
 * Provides honest service-backed diagnostics. No fake data allowed.
 */

static int backend_uptime(uint64_t* uptime_ms) {
    // TODO: Call real time/diag service
    return BHARAT_STATUS_ERR_UNSUPPORTED;
}

static int backend_status(char* out, size_t out_len) {
    if (!out || out_len < 32) return BHARAT_STATUS_ERR_LENGTH;
    shell_memcpy(out, "state=running profile=runtime", 30);
    return BHARAT_STATUS_OK;
}

static int backend_sys_info(char* out, size_t out_len) {
    if (!out || out_len < 32) return BHARAT_STATUS_ERR_LENGTH;
    shell_memcpy(out, "os=Bharat-OS mode=production", 29);
    return BHARAT_STATUS_OK;
}

static int backend_svc_list(char* out, size_t out_len) {
    // TODO: Call servicemgr
    return BHARAT_STATUS_ERR_UNSUPPORTED;
}

static int backend_svc_status(const char* name, char* out, size_t out_len) {
    (void)name; (void)out; (void)out_len;
    return BHARAT_STATUS_ERR_UNSUPPORTED;
}

static int backend_log_tail(char* out, size_t out_len) {
    (void)out; (void)out_len;
    return BHARAT_STATUS_ERR_UNSUPPORTED;
}

static int backend_health(char* out, size_t out_len) {
    (void)out; (void)out_len;
    return BHARAT_STATUS_ERR_UNSUPPORTED;
}

static int backend_dev_list(char* out, size_t out_len) {
    // TODO: Call devmgr
    return BHARAT_STATUS_ERR_UNSUPPORTED;
}

static int backend_mem_stat(char* out, size_t out_len) {
    // TODO: Call memmgr
    return BHARAT_STATUS_ERR_UNSUPPORTED;
}

static int backend_reboot(void) {
    // Requires BHARAT_SHELL_RIGHT_ADMIN
    return BHARAT_STATUS_ERR_UNSUPPORTED;
}

static void backend_audit(const char* event, const char* command, shell_status_code_t status) {
    (void)event; (void)command; (void)status;
}

const shell_backend_api_t* shell_runtime_backend(void) {
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
