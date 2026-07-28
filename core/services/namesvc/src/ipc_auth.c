#include "ipc_auth.h"
#include <bharat/cap/cap_validate.h>
#include <bharat/uapi/namesvc/contract.h>
#include <bharat/uapi/ipc/status.h>

#include <bharat/cap/cap_authz.h>

static const bharat_service_authz_desc_t namesvc_authz_descs[] = {
    {
        .opcode = BHARAT_NAMESVC_OP_LOOKUP,
        .object_type = BHARAT_CAP_OBJ_SERVICE,
        .required_rights = BHARAT_CAP_RIGHT_READ,
    },
    {
        .opcode = BHARAT_NAMESVC_OP_REGISTER,
        .object_type = BHARAT_CAP_OBJ_SERVICE,
        .required_rights = BHARAT_CAP_RIGHT_WRITE,
    },
    {
        .opcode = BHARAT_NAMESVC_OP_UNREGISTER,
        .object_type = BHARAT_CAP_OBJ_SERVICE,
        .required_rights = BHARAT_CAP_RIGHT_WRITE,
    },
    {
        .opcode = BHARAT_NAMESVC_OP_LIST_INTERFACES,
        .object_type = BHARAT_CAP_OBJ_SERVICE,
        .required_rights = BHARAT_CAP_RIGHT_WRITE,
    }
};

int namesvc_authorize(uint32_t opcode, bharat_cap_handle_t caller_cap) {
    // For lookup, we don't strictly require a capability transfer if it's just a query.
    if (opcode == BHARAT_NAMESVC_OP_LOOKUP && caller_cap == BHARAT_CAP_INVALID_HANDLE) {
        return BHARAT_IPC_STATUS_OK;
    }

    if (caller_cap == BHARAT_CAP_INVALID_HANDLE) {
        return BHARAT_IPC_STATUS_ERR_PERM;
    }

    return bharat_service_dispatch_authorize(
        0x00010002, // NAMESVC SERVICE ID
        opcode,
        namesvc_authz_descs,
        sizeof(namesvc_authz_descs) / sizeof(namesvc_authz_descs[0]),
        caller_cap,
        0
    );
}
