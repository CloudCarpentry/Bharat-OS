#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "ipc_dispatch.h"
#include <bharat/network/netmgr_ipc.h>
#include "bharat/component_version.h"
#include "bharat/buildinfo.h"

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

#include <bharat/ipc/ipc.h>

#include <bharat/runtime/freestanding_string.h>
#include <bharat/syscalls.h>

// Static service state
static bharat_ipc_endpoint_t g_netmgr_endpoint = BHARAT_CAP_INVALID_HANDLE;
static uint32_t g_health_ticks = 0;

static int netmgr_bind_endpoint(void) {
    // TODO(PR3.1-RUNTIME): Wire this to the real namesvc/registry once the API is available.
    // For now, this cleanly returns a startup failure instead of hiding behind a mock.
    return -1; // Missing registry API
}

static void service_poll_ipc(void) {
    bharat_ipc_msg_header_t hdr;
    netmgr_ipc_req_t req;
    netmgr_ipc_res_t res;

    memset(&hdr, 0, sizeof(hdr));
    memset(&req, 0, sizeof(req));
    memset(&res, 0, sizeof(res));

    // Non-blocking or blocking receive; stubs might just return an error if no message.
    int ret = bharat_ipc_recv(g_netmgr_endpoint, &hdr, &req, sizeof(req));
    if (ret == 0) {
        netmgr_ipc_handle_request(&hdr, &req, &res);

        // Send reply back using the transfer capability provided by the caller
        if (bharat_cap_is_valid(hdr.capability_transfer)) {
            bharat_ipc_msg_header_t rep_hdr;
            memset(&rep_hdr, 0, sizeof(rep_hdr));
            rep_hdr.message_id = hdr.message_id;
            rep_hdr.payload_size = sizeof(res);
            bharat_ipc_send(hdr.capability_transfer, &rep_hdr, &res);
        }
    } else {
        // Yield if no message received, until we have a real blocking wait/poll.
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
        // Log fatal exit reason once before shutdown
        // TODO(PR3.1-RUNTIME): Replace with proper service logger when available
        // printf("netmgr: FATAL - failed to bind endpoint\n");
        return -1;
    }

    netmgr_ipc_dispatch_init();
    netmgr_event_loop();
    return 0;
}
