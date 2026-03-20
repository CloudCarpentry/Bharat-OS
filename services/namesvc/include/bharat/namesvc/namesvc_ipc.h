#ifndef BHARAT_NAMESVC_IPC_H
#define BHARAT_NAMESVC_IPC_H

#include <stdint.h>
#include <stdbool.h>
#include <bharat/cap/cap.h>

#define NAMESVC_MAX_NAME_LEN 64

/**
 * @file namesvc_ipc.h
 * @brief IPC protocol definitions for the Name Service.
 */

typedef enum {
    NAMESVC_OP_INVALID = 0,
    NAMESVC_OP_REGISTER = 1,
    NAMESVC_OP_LOOKUP = 2,
    NAMESVC_OP_REMOVE = 3
} namesvc_op_t;

typedef enum {
    NAMESVC_STATUS_OK = 0,
    NAMESVC_STATUS_ERR_INVAL = -1,
    NAMESVC_STATUS_ERR_NOTFOUND = -2,
    NAMESVC_STATUS_ERR_NOSPACE = -3,
    NAMESVC_STATUS_ERR_EXISTS = -4
} namesvc_status_t;

struct namesvc_req_register {
    char name[NAMESVC_MAX_NAME_LEN];
};

struct namesvc_req_lookup {
    char name[NAMESVC_MAX_NAME_LEN];
};

struct namesvc_req_remove {
    char name[NAMESVC_MAX_NAME_LEN];
};

typedef struct {
    uint32_t opcode;
    uint32_t reserved;
    union {
        struct namesvc_req_register reg;
        struct namesvc_req_lookup lookup;
        struct namesvc_req_remove remove;
        uint8_t raw[112];
    } u;
} namesvc_ipc_req_t;

typedef struct {
    int32_t status;
    uint32_t reserved;
    union {
        struct {
            bharat_cap_handle_t endpoint;
        } lookup_res;
        uint8_t raw[120];
    } u;
} namesvc_ipc_res_t;

#endif // BHARAT_NAMESVC_IPC_H
