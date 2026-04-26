#include <ipc/ipc_status.h>
#include <stddef.h>

const char *bharat_ipc_status_to_string(bharat_status_t status)
{
    switch (status) {
        case BHARAT_IPC_STATUS_OK:
            return "IPC_OK";
        case BHARAT_IPC_STATUS_ERR_DECODE:
            return "IPC_ERR_DECODE";
        case BHARAT_IPC_STATUS_ERR_VERSION:
            return "IPC_ERR_VERSION";
        case BHARAT_IPC_STATUS_ERR_OPCODE:
            return "IPC_ERR_OPCODE";
        case BHARAT_IPC_STATUS_ERR_PERM:
            return "IPC_ERR_PERMISSION";
        case BHARAT_IPC_STATUS_ERR_NOT_FOUND:
            return "IPC_ERR_NOT_FOUND";
        case BHARAT_IPC_STATUS_ERR_UNSUPPORTED:
            return "IPC_ERR_UNSUPPORTED";
        case BHARAT_IPC_STATUS_ERR_INTERNAL:
            return "IPC_ERR_INTERNAL";
        case BHARAT_IPC_STATUS_ERR_TRUNCATED:
            return "IPC_ERR_TRUNCATED";
        case BHARAT_IPC_STATUS_ERR_LENGTH:
            return "IPC_ERR_LENGTH";
        case BHARAT_IPC_STATUS_ERR_FLAGS:
            return "IPC_ERR_FLAGS";
        default:
            return "IPC_ERR_UNKNOWN";
    }
}
