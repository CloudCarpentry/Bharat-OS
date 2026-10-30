#ifndef BHARAT_UAPI_NAMESVC_CONTRACT_H
#define BHARAT_UAPI_NAMESVC_CONTRACT_H

#include <bharat/uapi/abi_types.h>
#include <bharat/uapi/ipc/contract.h>
#include <bharat/uapi/services/service_ids.h>
#include <stdbool.h>
#include <stdint.h>

/*
 * Bounded to 48 bytes so that namesvc_ipc_req_t (8 + 64 = 72 bytes) +
 * bharat_ipc_contract_header_t (56 bytes) = 128 bytes fits within the
 * default/MMU-Lite endpoint payload budget.
 */
#define NAMESVC_MAX_NAME_LEN 48

/**
 * @file contract.h
 * @brief IPC protocol definitions for the Name Service.
 */

typedef enum {
  BHARAT_NAMESVC_OP_INVALID = 0,
  BHARAT_NAMESVC_OP_REGISTER = 1,
  BHARAT_NAMESVC_OP_LOOKUP = 2,
  BHARAT_NAMESVC_OP_UNREGISTER = 3,
  BHARAT_NAMESVC_OP_LIST_INTERFACES = 4
} namesvc_op_t;

typedef enum {
  NAMESVC_STATUS_OK = 0,
  NAMESVC_STATUS_ERR_INVAL = -1,
  NAMESVC_STATUS_ERR_NOTFOUND = -2,
  NAMESVC_STATUS_ERR_NOSPACE = -3,
  NAMESVC_STATUS_ERR_EXISTS = -4,
  NAMESVC_STATUS_ERR_COMPAT = -5,
  NAMESVC_STATUS_ERR_UNSUPPORTED = -6
} namesvc_status_t;

struct namesvc_req_register {
  char service_name[NAMESVC_MAX_NAME_LEN];
  bharat_service_id_t service_id;
  uint32_t interface_version;
  uint32_t transport_flags;
};

struct namesvc_req_lookup {
  char service_name[NAMESVC_MAX_NAME_LEN];
  uint32_t requested_version; // Requested version
  bool exact_version; // True if exact version required, false for compatible
};

struct namesvc_req_unregister {
  char service_name[NAMESVC_MAX_NAME_LEN];
};

typedef struct {
  uint32_t opcode;
  uint32_t reserved;
  union {
    struct namesvc_req_register reg;
    struct namesvc_req_lookup lookup;
    struct namesvc_req_unregister unreg;
    uint8_t raw[64];
  } u;
} namesvc_ipc_req_t;

typedef struct {
  int32_t status;
  uint32_t reserved;
  union {
    struct {
      bharat_handle_t endpoint;
      bharat_service_id_t service_id;
      uint32_t interface_version;
      uint32_t transport_flags;
    } lookup_res;
    uint8_t raw[64];
  } u;
} namesvc_ipc_res_t;

#if !defined(Profile_RTOS) && !defined(BHARAT_PROFILE_RTOS)
_Static_assert(sizeof(namesvc_ipc_req_t) <= BHARAT_IPC_CONTRACT_PAYLOAD_MAX,
               "namesvc_ipc_req_t exceeds endpoint contract payload budget");
_Static_assert(sizeof(namesvc_ipc_res_t) <= BHARAT_IPC_CONTRACT_PAYLOAD_MAX,
               "namesvc_ipc_res_t exceeds endpoint contract payload budget");
#endif

#endif // BHARAT_UAPI_NAMESVC_CONTRACT_H
