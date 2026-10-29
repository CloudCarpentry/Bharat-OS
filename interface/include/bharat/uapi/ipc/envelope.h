#ifndef BHARAT_UAPI_IPC_ENVELOPE_H
#define BHARAT_UAPI_IPC_ENVELOPE_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file envelope.h
 * @brief Canonical authenticated IPC receive envelope.
 */

#define BH_IPC_RECEIVE_ENVELOPE_VERSION_V1 1U

/* Validity bitflags for bh_ipc_receive_envelope_v1_t */
enum {
    BH_IPC_ENVELOPE_F_SENDER_PID            = (1ULL << 0),
    BH_IPC_ENVELOPE_F_SENDER_PROCESS_OBJECT = (1ULL << 1),
    BH_IPC_ENVELOPE_F_SENDER_CSPACE         = (1ULL << 2),
    BH_IPC_ENVELOPE_F_SENDER_OWNER_CORE     = (1ULL << 3),
    BH_IPC_ENVELOPE_F_BOOT_SESSION          = (1ULL << 4),
    BH_IPC_ENVELOPE_F_SERVICE_INCARNATION   = (1ULL << 5),
    BH_IPC_ENVELOPE_F_AUTHORITY_CAP         = (1ULL << 6),
    BH_IPC_ENVELOPE_F_REPLY_SEND_CAP        = (1ULL << 7),
    BH_IPC_ENVELOPE_F_MESSAGE_ID            = (1ULL << 8),
    BH_IPC_ENVELOPE_F_DELIVERY_GENERATION   = (1ULL << 9),
};

typedef struct bh_ipc_receive_envelope_v1 {
    uint32_t version;                  /* offset 0 */
    uint32_t struct_size;              /* offset 4 */
    uint64_t valid_fields;             /* offset 8 */
    uint64_t sender_pid;               /* offset 16 */
    uint64_t sender_process_object_id; /* offset 24 */
    uint64_t boot_session_id;          /* offset 32 */
    uint64_t service_incarnation_id;   /* offset 40 */
    uint32_t sender_cspace_id;         /* offset 48 */
    uint32_t sender_owner_core;        /* offset 52 */
    uint32_t authority_cap;            /* offset 56, receiver-local */
    uint32_t reply_send_cap;           /* offset 60, receiver-local */
    uint32_t authority_object_type;    /* offset 64 */
    uint32_t reserved0;                /* offset 68, zero */
    uint64_t authority_rights;         /* offset 72 */
    uint64_t message_id;               /* offset 80, correlation only */
    uint64_t delivery_generation;      /* offset 88 */
} bh_ipc_receive_envelope_v1_t;

#ifdef __cplusplus
static_assert(sizeof(bh_ipc_receive_envelope_v1_t) == 96U, "receive envelope v1 wire size");
static_assert(offsetof(bh_ipc_receive_envelope_v1_t, version) == 0U, "offset version");
static_assert(offsetof(bh_ipc_receive_envelope_v1_t, struct_size) == 4U, "offset struct_size");
static_assert(offsetof(bh_ipc_receive_envelope_v1_t, valid_fields) == 8U, "offset valid_fields");
static_assert(offsetof(bh_ipc_receive_envelope_v1_t, sender_pid) == 16U, "offset sender_pid");
static_assert(offsetof(bh_ipc_receive_envelope_v1_t, sender_process_object_id) == 24U, "offset sender_process_object_id");
static_assert(offsetof(bh_ipc_receive_envelope_v1_t, boot_session_id) == 32U, "offset boot_session_id");
static_assert(offsetof(bh_ipc_receive_envelope_v1_t, service_incarnation_id) == 40U, "offset service_incarnation_id");
static_assert(offsetof(bh_ipc_receive_envelope_v1_t, sender_cspace_id) == 48U, "offset sender_cspace_id");
static_assert(offsetof(bh_ipc_receive_envelope_v1_t, sender_owner_core) == 52U, "offset sender_owner_core");
static_assert(offsetof(bh_ipc_receive_envelope_v1_t, authority_cap) == 56U, "offset authority_cap");
static_assert(offsetof(bh_ipc_receive_envelope_v1_t, reply_send_cap) == 60U, "offset reply_send_cap");
static_assert(offsetof(bh_ipc_receive_envelope_v1_t, authority_object_type) == 64U, "offset authority_object_type");
static_assert(offsetof(bh_ipc_receive_envelope_v1_t, reserved0) == 68U, "offset reserved0");
static_assert(offsetof(bh_ipc_receive_envelope_v1_t, authority_rights) == 72U, "offset authority_rights");
static_assert(offsetof(bh_ipc_receive_envelope_v1_t, message_id) == 80U, "offset message_id");
static_assert(offsetof(bh_ipc_receive_envelope_v1_t, delivery_generation) == 88U, "offset delivery_generation");
#else
_Static_assert(sizeof(bh_ipc_receive_envelope_v1_t) == 96U, "receive envelope v1 wire size");
_Static_assert(offsetof(bh_ipc_receive_envelope_v1_t, version) == 0U, "offset version");
_Static_assert(offsetof(bh_ipc_receive_envelope_v1_t, struct_size) == 4U, "offset struct_size");
_Static_assert(offsetof(bh_ipc_receive_envelope_v1_t, valid_fields) == 8U, "offset valid_fields");
_Static_assert(offsetof(bh_ipc_receive_envelope_v1_t, sender_pid) == 16U, "offset sender_pid");
_Static_assert(offsetof(bh_ipc_receive_envelope_v1_t, sender_process_object_id) == 24U, "offset sender_process_object_id");
_Static_assert(offsetof(bh_ipc_receive_envelope_v1_t, boot_session_id) == 32U, "offset boot_session_id");
_Static_assert(offsetof(bh_ipc_receive_envelope_v1_t, service_incarnation_id) == 40U, "offset service_incarnation_id");
_Static_assert(offsetof(bh_ipc_receive_envelope_v1_t, sender_cspace_id) == 48U, "offset sender_cspace_id");
_Static_assert(offsetof(bh_ipc_receive_envelope_v1_t, sender_owner_core) == 52U, "offset sender_owner_core");
_Static_assert(offsetof(bh_ipc_receive_envelope_v1_t, authority_cap) == 56U, "offset authority_cap");
_Static_assert(offsetof(bh_ipc_receive_envelope_v1_t, reply_send_cap) == 60U, "offset reply_send_cap");
_Static_assert(offsetof(bh_ipc_receive_envelope_v1_t, authority_object_type) == 64U, "offset authority_object_type");
_Static_assert(offsetof(bh_ipc_receive_envelope_v1_t, reserved0) == 68U, "offset reserved0");
_Static_assert(offsetof(bh_ipc_receive_envelope_v1_t, authority_rights) == 72U, "offset authority_rights");
_Static_assert(offsetof(bh_ipc_receive_envelope_v1_t, message_id) == 80U, "offset message_id");
_Static_assert(offsetof(bh_ipc_receive_envelope_v1_t, delivery_generation) == 88U, "offset delivery_generation");
#endif

#ifdef __cplusplus
}
#endif

#endif /* BHARAT_UAPI_IPC_ENVELOPE_H */
