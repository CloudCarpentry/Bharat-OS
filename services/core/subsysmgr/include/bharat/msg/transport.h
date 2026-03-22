#ifndef BHARAT_MSG_TRANSPORT_H
#define BHARAT_MSG_TRANSPORT_H

#include "wire.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Transport Capability Flags
// ============================================================================

#define BHARAT_TRANSPORT_CAP_RELIABLE    (1U << 0)
#define BHARAT_TRANSPORT_CAP_ORDERED     (1U << 1)
#define BHARAT_TRANSPORT_CAP_OOL         (1U << 2)
#define BHARAT_TRANSPORT_CAP_CAPS        (1U << 3)
#define BHARAT_TRANSPORT_CAP_FRAGMENT    (1U << 4)
#define BHARAT_TRANSPORT_CAP_LOCAL_ONLY  (1U << 5)
#define BHARAT_TRANSPORT_CAP_SHMEM       (1U << 6)

// ============================================================================
// Transport Abstract Operations
// ============================================================================

struct bharat_transport;

typedef struct {
    /**
     * @brief Send a fully serialized message (header + payload) over the transport.
     */
    int (*send)(struct bharat_transport* self, const uint8_t* buf, size_t len);

    /**
     * @brief Block/Poll to receive a message.
     */
    int (*recv)(struct bharat_transport* self, uint8_t* buf, size_t cap, size_t* out_len);

    /**
     * @brief Returns the capabilities supported by this transport.
     */
    uint32_t (*get_caps)(struct bharat_transport* self);

    /**
     * @brief Returns the Maximum Transmission Unit (MTU) size for inline payloads.
     */
    size_t (*get_mtu)(struct bharat_transport* self);

    /**
     * @brief Wait for data to be ready (optional).
     */
    int (*poll)(struct bharat_transport* self, int timeout_ms);

    /**
     * @brief Acknowledge receipt if transport requires explicit ACK.
     */
    int (*ack)(struct bharat_transport* self, uint64_t request_id);

    /**
     * @brief Close the transport connection/binding.
     */
    int (*close)(struct bharat_transport* self);

} bharat_transport_ops_t;

// ============================================================================
// Transport Instance
// ============================================================================

typedef struct bharat_transport {
    const bharat_transport_ops_t* ops;
    void* ctx;         // Private pointer for the specific transport backend
    uint32_t local_id; // Identifies this endpoint
} bharat_transport_t;

// ============================================================================
// Concrete Bindings Declarations (Implemented elsewhere)
// ============================================================================

/**
 * @brief Create a local in-memory loopback transport for tests.
 */
int bharat_transport_loopback_create(bharat_transport_t* t, size_t max_mtu);

#ifdef __cplusplus
}
#endif

#endif // BHARAT_MSG_TRANSPORT_H
