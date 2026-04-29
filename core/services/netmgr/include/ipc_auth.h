#ifndef NETMGR_IPC_AUTH_H
#define NETMGR_IPC_AUTH_H

#include <stdint.h>
#include <stdbool.h>
#include <bharat/cap/cap.h>
#include "netmgr_auth_core.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <bharat/cap/cap_validate.h>

// Define rights
#define BHARAT_CAP_RIGHT_NET_READ_STATS      (1ull << 0)
#define BHARAT_CAP_RIGHT_NET_IFACE_QUERY     (1ull << 1)
#define BHARAT_CAP_RIGHT_NET_IFACE_CONFIG    (1ull << 2)
#define BHARAT_CAP_RIGHT_NET_ROUTE_QUERY     (1ull << 3)
#define BHARAT_CAP_RIGHT_NET_ROUTE_MUTATE    (1ull << 4)
#define BHARAT_CAP_RIGHT_NET_FIREWALL_READ   (1ull << 5)
#define BHARAT_CAP_RIGHT_NET_FIREWALL_WRITE  (1ull << 6)
#define BHARAT_CAP_RIGHT_NET_ADMIN           (1ull << 7)
#define BHARAT_CAP_RIGHT_NET_NEIGHBOR_QUERY  (1ull << 8)
#define BHARAT_CAP_RIGHT_NET_DRIVER_QUERY    (1ull << 9)
#define BHARAT_CAP_RIGHT_NET_DRIVER_ADMIN    (1ull << 10)

// Set the current caller capability (mock context for now)
void netmgr_set_caller_cap(bharat_cap_handle_t cap);

// Audit log structure for tests and observability
typedef struct {
    uint32_t opcode;
    bharat_cap_handle_t caller_cap;
    bharat_cap_object_type_t object_type;
    uint64_t object_id;
    uint64_t required_rights;
    bharat_cap_scope_t required_scope;
    bharat_cap_status_t status;
    bool valid;
} netmgr_audit_entry_t;

// Uniform authorization hook
int netmgr_authorize(
    bharat_cap_handle_t caller_cap,
    bharat_cap_object_type_t object_type,
    uint64_t object_id,
    uint64_t required_rights,
    const bharat_cap_scope_t *required_scope);

// Test-only: Get the last audit entry
#ifdef BHARAT_BUILD_HOST_TESTS
const netmgr_audit_entry_t* netmgr_get_last_audit(void);
void netmgr_clear_last_audit(void);
#endif

// Re-map the unified audit hook to the core logic results if needed
void netmgr_audit_core_failure(const netmgr_auth_audit_t *audit);

#ifdef __cplusplus
}
#endif

#endif // NETMGR_IPC_AUTH_H
