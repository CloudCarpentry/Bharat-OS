extern int bharat_write(int fd, const void* buf, unsigned long count);
static void console_print(const char* str) {
    int len = 0;
    while(str[len]) len++;
    bharat_write(1, str, len);
}
static int my_strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) { s1++; s2++; }
    return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}
#include <bharat/uapi/device/bharat_device_accelmgr_v2_types.h>
#include <bharat/uapi/services/service_ids.h>
#include <bharat/ipc/ipc.h>
// removed cap_alloc.h

void print_execplan_usage(void) {
    console_print("Usage:\n");
    console_print("  execplan stats   - Display execution plan telemetry\n");
}

int main(int argc, char **argv) {
    if (argc < 2) {
        print_execplan_usage();
        return 1;
    }

    if (my_strcmp(argv[1], "stats") == 0) {
        // Prepare request
        bharat_ipc_msg_header_t hdr = {
            .service_id = BHARAT_SERVICE_ACCELMGR_V2,
            .opcode = 17, // GetTelemetrySnapshot
            .payload_size = sizeof(bharat_device_accelmgr_v2_GetTelemetrySnapshotReq_t),
            .capability_transfer = BHARAT_CAP_INVALID_HANDLE // Would be actual cap handle in real environment
        };
        bharat_device_accelmgr_v2_GetTelemetrySnapshotReq_t req = {0};

        bharat_device_accelmgr_v2_GetTelemetrySnapshotResp_t resp = {0};

        // This is a stub for shell environment. In a real shell execution,
        // we'd use bh_ipc_call to BHARAT_SERVICE_ACCELMGR_V2 endpoint.

        console_print("--- Execution Plan Telemetry ---\n");
        console_print("See JSON diagnostics for detailed metrics.\n");
        console_print("--------------------------------\n");
    } else {
        console_print("Unknown command: "); console_print(argv[1]); console_print("\n");
        print_execplan_usage();
        return 1;
    }

    return 0;
}
