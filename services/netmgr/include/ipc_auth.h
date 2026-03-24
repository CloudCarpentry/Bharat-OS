#ifndef NETMGR_IPC_AUTH_H
#define NETMGR_IPC_AUTH_H

#include <stdint.h>
#include <stdbool.h>
#include <bharat/cap/cap.h>

#ifdef __cplusplus
extern "C" {
#endif

// Define rights
#define NETMGR_RIGHT_READ  (1 << 0)
#define NETMGR_RIGHT_WRITE (1 << 1)
#define NETMGR_RIGHT_ADMIN (1 << 2)

// Set the current caller capability (mock context for now)
void netmgr_set_caller_cap(bharat_cap_handle_t cap);

// Uniform authorization hook
int netmgr_authorize(uint32_t opcode, bharat_cap_handle_t caller_cap, uint32_t target_if_id, uint32_t required_rights);

#ifdef __cplusplus
}
#endif

#endif // NETMGR_IPC_AUTH_H
