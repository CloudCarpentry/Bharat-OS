#ifndef BHARAT_UAPI_IPC_CONTRACT_H
#define BHARAT_UAPI_IPC_CONTRACT_H

#include <bharat/uapi/ipc/envelope.h>
#include <stddef.h>
#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file contract.h
 * @brief Canonical service IPC contract envelope.
 */

/* Service IPC Contract Header */
typedef struct {
  uint32_t header_version;
  uint32_t service_id;
  uint32_t interface_id;
  uint32_t interface_version;
  uint32_t opcode;
  uint32_t flags;
  uint32_t payload_size;
  int32_t status;
  uint64_t message_id;
  /* Legacy serialized claims, never authenticated handles or authority.
   * Kernel-delivered transfer metadata must be separate from this header. */
  uint64_t capability_transfer;
  uint64_t reply_endpoint;
} bharat_ipc_contract_header_t;

#ifdef __cplusplus
static_assert(sizeof(bharat_ipc_contract_header_t) == 56U,
              "IPC V1 header size");
static_assert(offsetof(bharat_ipc_contract_header_t, message_id) == 32U,
              "IPC V1 message ID offset");
static_assert(offsetof(bharat_ipc_contract_header_t, capability_transfer) ==
                  40U,
              "IPC V1 legacy transfer offset");
static_assert(offsetof(bharat_ipc_contract_header_t, reply_endpoint) == 48U,
              "IPC V1 legacy reply offset");
#else
_Static_assert(sizeof(bharat_ipc_contract_header_t) == 56U,
               "IPC V1 header size");
_Static_assert(offsetof(bharat_ipc_contract_header_t, message_id) == 32U,
               "IPC V1 message ID offset");
_Static_assert(offsetof(bharat_ipc_contract_header_t, capability_transfer) ==
                   40U,
               "IPC V1 legacy transfer offset");
_Static_assert(offsetof(bharat_ipc_contract_header_t, reply_endpoint) == 48U,
               "IPC V1 legacy reply offset");
#endif

enum {
  BHARAT_IPC_HEADER_VERSION_V1 = 1u,
};

enum {
  BHARAT_IPC_FLAG_REPLY = (1u << 0),
  BHARAT_IPC_FLAG_NONBLOCK = (1u << 1),
  BHARAT_IPC_FLAG_CAP_TRANSFER = (1u << 2),
};

#define BHARAT_IPC_FLAGS_ALL_V1                                                \
  (BHARAT_IPC_FLAG_REPLY | BHARAT_IPC_FLAG_NONBLOCK |                          \
   BHARAT_IPC_FLAG_CAP_TRANSFER)

/* Authoritative profile-aware endpoint payload ceiling shared by kernel and
 * userland. */
#ifndef BHARAT_IPC_ENDPOINT_PAYLOAD_MAX
#if defined(Profile_RTOS) ||                                                   \
    (defined(BHARAT_PROFILE_RTOS) && BHARAT_PROFILE_RTOS)
#define BHARAT_IPC_ENDPOINT_PAYLOAD_MAX 64U
#elif defined(FEATURE_NUMA_AWARE)
#define BHARAT_IPC_ENDPOINT_PAYLOAD_MAX 256U
#else
#define BHARAT_IPC_ENDPOINT_PAYLOAD_MAX 128U
#endif
#endif

#define BHARAT_IPC_CONTRACT_PAYLOAD_MAX                                        \
  (BHARAT_IPC_ENDPOINT_PAYLOAD_MAX -                                           \
   (uint32_t)sizeof(bharat_ipc_contract_header_t))

/* Endpoint capability operations on BH_SYS_CAPABILITY_INVOKE */
#define BH_ENDPOINT_OP_CLOSE 1U
#define BH_ENDPOINT_OP_CANCEL_WAITER 2U

#ifdef __cplusplus
}
#endif

#endif // BHARAT_UAPI_IPC_CONTRACT_H
