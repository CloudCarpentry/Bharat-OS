#ifndef BHARAT_BOOTSTRAP_SERVICE_LAUNCH_H
#define BHARAT_BOOTSTRAP_SERVICE_LAUNCH_H
#include <stdint.h>
#include <stddef.h>

/* Operations of the existing CAPABILITY_INVOKE syscall, not syscall numbers. */
#define BH_BOOTSTRAP_OP_LAUNCH 1U
#define BH_BOOTSTRAP_OP_PROBE 2U
#define BH_BOOTSTRAP_OP_CREATE_IMAGE 3U
#define BH_PROCESS_OP_START 4U
#define BH_PROCESS_OP_QUERY 5U
#define BH_PROCESS_OP_REAP 6U
#define BH_PROCESS_OP_TERMINATE 7U
#define BH_BOOTSTRAP_IMAGE_MAX (1024U * 1024U)
#define BH_BOOTSTRAP_SERVICE_ABI 1U
#define BH_BOOTSTRAP_EVENT_BOUND 1U
#define BH_BOOTSTRAP_EVENT_READY 2U
#define BH_BOOTSTRAP_EVENT_FAILED 3U

typedef struct {
    uint32_t version;
    uint32_t service_id;
    uint32_t namesvc_cap;
    uint32_t delegate_launch;
    char module_name[32];
} bh_bootstrap_launch_request_t;

typedef struct {
    uint64_t process_id;
    uint32_t process_cap;
    uint32_t event_receive_cap;
    uint32_t service_send_cap;
    uint32_t reserved;
} bh_bootstrap_launch_result_t;

typedef struct {
    uint32_t version;
    uint32_t type;
    uint32_t service_id;
    int32_t status;
} bh_bootstrap_service_event_t;

/* User addresses here are syscall-only carriers, never IPC/wire pointers. */
typedef struct {
    uint64_t image_address;
    uint64_t image_size;
    uint32_t version;
    uint32_t priority;
    char name[32];
} bh_process_image_request_t;
typedef struct {
    uint64_t process_id;
    uint64_t thread_id;
    uint64_t entry_point;
    uint32_t process_cap;
    uint32_t reserved;
} bh_process_image_result_t;
_Static_assert(sizeof(bh_process_image_request_t) == 56, "image request ABI");
_Static_assert(sizeof(bh_process_image_result_t) == 32, "image result ABI");

_Static_assert(sizeof(bh_bootstrap_launch_request_t) == 48, "launch ABI size");
_Static_assert(sizeof(bh_bootstrap_launch_result_t) == 24, "result ABI size");
_Static_assert(sizeof(bh_bootstrap_service_event_t) == 16, "event ABI size");
_Static_assert(offsetof(bh_bootstrap_launch_request_t, module_name) == 16, "launch name offset");
_Static_assert(offsetof(bh_bootstrap_launch_result_t, process_cap) == 8, "process cap offset");
_Static_assert(offsetof(bh_bootstrap_launch_result_t, event_receive_cap) == 12, "event cap offset");
_Static_assert(offsetof(bh_bootstrap_service_event_t, service_id) == 8, "event sender offset");
_Static_assert(offsetof(bh_bootstrap_service_event_t, status) == 12, "event status offset");
_Static_assert(offsetof(bh_process_image_request_t, version) == 16, "image version offset");
_Static_assert(offsetof(bh_process_image_request_t, name) == 24, "image name offset");
_Static_assert(offsetof(bh_process_image_result_t, process_cap) == 24, "image cap offset");
#endif
