#ifndef NETMGR_IPC_CONTRACT_H
#define NETMGR_IPC_CONTRACT_H

#include <stdint.h>
#include <stddef.h>
#include <bharat/network/netmgr_ipc.h>
#include <bharat/ipc/ipc.h>

#ifdef __cplusplus
extern "C" {
#endif

// Handler function pointer
typedef void (*netmgr_op_handler_t)(const netmgr_ipc_req_t *req, netmgr_ipc_res_t *res);

#include <bharat/cap/cap_validate.h>

// Operation Descriptor Table Entry
typedef struct {
    uint32_t opcode;
    const char *name;
    size_t min_request_size;
    size_t max_request_size;
    bharat_cap_object_type_t object_type;
    uint64_t required_rights;
    uint32_t (*extract_target_obj)(const netmgr_ipc_req_t *req);
    netmgr_op_handler_t handler;
} netmgr_op_descriptor_t;

// Get the descriptor for an opcode
const netmgr_op_descriptor_t* netmgr_get_op_descriptor(uint32_t opcode);

#ifdef __cplusplus
}
#endif

#endif // NETMGR_IPC_CONTRACT_H
