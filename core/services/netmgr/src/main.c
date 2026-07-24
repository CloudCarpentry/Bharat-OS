#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "ipc_dispatch.h"
#include <bharat/network/netmgr_ipc.h>
#include "bharat/component_version.h"
#include "bharat/buildinfo.h"

#include <bharat/ipc/ipc.h>
#include <bharat/service/service_runtime.h>
#include <bharat/namesvc/client.h>
#include <bharat/uapi/services/service_ids.h>
#include <bharat/runtime/freestanding_string.h>
#include <bharat/syscalls.h>

BHARAT_REGISTER_COMPONENT(
    BHARAT_COMPONENT_NAME,
    BHARAT_COMPONENT_KIND,
    BHARAT_COMPONENT_VERSION,
    BHARAT_COMPONENT_IFACE,
    0, /* abi version */
    BHARAT_COMPONENT_CHANNEL,
    BHARAT_GIT_SHA,
    BHARAT_GIT_DIRTY,
    BHARAT_BUILD_EPOCH,
    BHARAT_BUILD_TIME_UTC
);

// Static service state
static bharat_ipc_endpoint_t g_netmgr_endpoint = BHARAT_CAP_INVALID_HANDLE;
static uint32_t g_health_ticks = 0;

static int netmgr_bind_endpoint(void) {
    // Create endpoint through service runtime
    g_netmgr_endpoint = service_runtime_create_endpoint(BHARAT_SERVICE_NETMGR, 0);
    if (!bharat_cap_is_valid(g_netmgr_endpoint)) {
        return -1;
    }

    // Register with namesvc
    // Version 1 is hardcoded as per requirement "Validate IPC Contract/Version (Version 1)" in ipc_dispatch.c
    int ret = namesvc_register("netmgr", BHARAT_SERVICE_NETMGR, g_netmgr_endpoint, 1, 0);
    if (ret != NAMESVC_STATUS_OK) {
        return -1;
    }

    return 0;
}

static void service_poll_ipc(void) {
    bharat_ipc_msg_header_t hdr;
    netmgr_ipc_req_t req;
    netmgr_ipc_res_t res;

    memset(&hdr, 0, sizeof(hdr));
    memset(&req, 0, sizeof(req));
    memset(&res, 0, sizeof(res));

    // Bounded receive - if no message, returns error and we yield
    int ret = bharat_ipc_recv(g_netmgr_endpoint, &hdr, &req, sizeof(req));
    if (ret == BHARAT_IPC_STATUS_OK) {
        netmgr_ipc_handle_request(&hdr, &req, &res);

        // Send reply back using the transfer capability provided by the caller
        if (bharat_cap_is_valid(hdr.reply_endpoint)) {
            bharat_ipc_msg_header_t rep_hdr;
            memset(&rep_hdr, 0, sizeof(rep_hdr));
            rep_hdr.header_version = BHARAT_IPC_HEADER_VERSION_V1;
            rep_hdr.message_id = hdr.message_id;
            rep_hdr.service_id = BHARAT_SERVICE_NETMGR;
            rep_hdr.opcode = req.opcode;
            rep_hdr.payload_size = sizeof(res);
            bharat_ipc_send(hdr.reply_endpoint, &rep_hdr, &res);
        }
    } else {
        // Yield if no message received, until we have a real blocking wait/poll.
        // TODO(SERVICE-RUNTIME): Replace bounded poll with blocking receive
        // once kernel IPC wait queues are production-ready.
        bharat_sched_yield();
    }
}

static void service_run_timers(void) {
    // Process timer and deadline events (e.g. ARP timeouts, health check intervals)
    g_health_ticks++;
}

static void service_handle_control_events(void) {
    // Process any driver health or restart events
}

static void service_run_deferred_work(void) {
    // Process deferred housekeeping tasks
}

void netmgr_event_loop(void) {
    while (1) {
        service_poll_ipc();
        service_run_timers();
        service_handle_control_events();
        service_run_deferred_work();
    }
}

int main(void) {
    if (netmgr_bind_endpoint() != 0) {
        return -1;
    }

    netmgr_ipc_dispatch_init();
    netmgr_event_loop();
    return 0;
}
