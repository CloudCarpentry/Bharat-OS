#ifndef NETMGR_IPC_AUTH_H
#define NETMGR_IPC_AUTH_H

#include <stdint.h>
#include <stdbool.h>
#include <bharat/cap/cap.h>

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

// Set the current caller capability (mock context for now)
void netmgr_set_caller_cap(bharat_cap_handle_t cap);

// Uniform authorization hook
int netmgr_authorize(
    bharat_cap_handle_t caller_cap,
    bharat_cap_object_type_t object_type,
    uint64_t object_id,
    uint64_t required_rights,
    const bharat_cap_scope_t *required_scope);

#ifdef __cplusplus
}
#endif

#endif // NETMGR_IPC_AUTH_H
