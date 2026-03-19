#include "bharat/msg/wire.h"
#include <stdint.h>
#include <stddef.h>

#include "bharat/msg/wire_types.h"

// For minimal integration of proxying over the wire, we can simulate an
// export and an import of a capability using a generic proxy mechanism

typedef struct {
    uint64_t local_proxy_id;
    uint64_t remote_cap_id;
    uint32_t remote_node;
} cap_proxy_obj_t;

int capwire_export_cap(uint64_t local_cap_id, bharat_cap_wire_t* out_wire) {
    if (!out_wire) return -1;

    // In a real system, we'd lookup the local_cap_id, check rights,
    // mark it as exported or create a lease, and serialize the details.

    // For this minimal integration:
    out_wire->cap_id = local_cap_id;
    out_wire->rights = 0xFFFFFFFF; // all rights
    out_wire->object_type = 1; // Generic type

    return 0;
}

int capwire_import_cap(const bharat_cap_wire_t* wire, uint32_t remote_node, cap_proxy_obj_t* out_proxy) {
    if (!wire || !out_proxy) return -1;

    // In a real system, we'd allocate a new local capability slot,
    // mark it as an imported proxy (CAP_OBJ_IMPORTED_PROXY), and bind
    // it to the remote node and cap_id.

    // For this minimal integration:
    out_proxy->local_proxy_id = 1000 + wire->cap_id; // Fake proxy ID
    out_proxy->remote_cap_id = wire->cap_id;
    out_proxy->remote_node = remote_node;

    return 0;
}

// Stubs for the generated dispatch code to call
#include "../../services/cap/generated/bharat_cap_v1_types.h"

int handle_remote_grant(const bharat_cap_v1_RemoteGrantReq_t* req, bharat_cap_v1_RemoteGrantResp_t* resp) {
    cap_proxy_obj_t proxy;
    int ret = capwire_import_cap(&req->cap, req->target_node, &proxy);

    if (ret == 0) {
        resp->status = 0; // OK
    } else {
        resp->status = 1; // ERR
    }

    return ret;
}
