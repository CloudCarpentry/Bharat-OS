#ifndef BHARAT_CAP_VALIDATE_H
#define BHARAT_CAP_VALIDATE_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BHARAT_CAP_OK = 0,
    BHARAT_CAP_INVALID,
    BHARAT_CAP_NOT_FOUND,
    BHARAT_CAP_REVOKED,
    BHARAT_CAP_RIGHTS_DENIED,
    BHARAT_CAP_SCOPE_DENIED,
    BHARAT_CAP_OBJECT_MISMATCH,
    BHARAT_CAP_EXPIRED,
    BHARAT_CAP_STALE,
    BHARAT_CAP_UNSUPPORTED,
} bharat_cap_status_t;

typedef enum {
    BHARAT_CAP_OBJ_NONE = 0,
    BHARAT_CAP_OBJ_SERVICE,
    BHARAT_CAP_OBJ_NET_IFACE,
    BHARAT_CAP_OBJ_ROUTE_TABLE,
    BHARAT_CAP_OBJ_ENDPOINT,
    BHARAT_CAP_OBJ_PROCESS,
    BHARAT_CAP_OBJ_VM_SPACE,
    BHARAT_CAP_OBJ_DEVICE,
    BHARAT_CAP_OBJ_DMA_DOMAIN,
} bharat_cap_object_type_t;

typedef enum {
    BHARAT_CAP_SCOPE_GLOBAL = 0,
    BHARAT_CAP_SCOPE_OBJECT,
    BHARAT_CAP_SCOPE_NAMESPACE,
    BHARAT_CAP_SCOPE_SERVICE,
} bharat_cap_scope_kind_t;

typedef struct {
    bharat_cap_scope_kind_t kind;
    uint64_t scope_id;
} bharat_cap_scope_t;

#include <bharat/cap/cap.h>

typedef struct {
    bharat_cap_handle_t handle;
    uint64_t cap_id;
    uint64_t parent_cap_id;
    uint64_t owner_principal_id;
    bharat_cap_object_type_t object_type;
    uint64_t object_id;
    uint64_t rights;
    bharat_cap_scope_t scope;
    uint64_t generation;
    uint64_t flags;
    uint32_t state;
} bharat_cap_descriptor_t;

typedef struct {
    bool allowed;
    bharat_cap_status_t status;
    bharat_cap_descriptor_t descriptor;
} bharat_cap_validation_result_t;

/**
 * @brief Validates a capability against an expected object, rights, and scope.
 *
 * @param handle The capability handle to validate.
 * @param expected_object_type The expected object type.
 * @param expected_object_id The expected object ID.
 * @param required_rights The required rights mask.
 * @param required_scope The required scope.
 * @param out_result Output parameter to store the validation result.
 * @return bharat_cap_status_t Status of the validation API call (not the boolean allowance).
 */
bharat_cap_status_t bharat_cap_validate(
    bharat_cap_handle_t handle,
    bharat_cap_object_type_t expected_object_type,
    uint64_t expected_object_id,
    uint64_t required_rights,
    const bharat_cap_scope_t *required_scope,
    bharat_cap_validation_result_t *out_result);

typedef bharat_cap_status_t (*bharat_cap_validate_fn_t)(
    bharat_cap_handle_t handle,
    bharat_cap_object_type_t expected_object_type,
    uint64_t expected_object_id,
    uint64_t required_rights,
    const bharat_cap_scope_t *required_scope,
    bharat_cap_validation_result_t *out_result);

/**
 * @brief Sets a test-specific backend for capability validation.
 *
 * @param fn The fake validation function.
 */
void bharat_cap_set_validate_backend_for_tests(bharat_cap_validate_fn_t fn);

#ifdef __cplusplus
}
#endif

#endif // BHARAT_CAP_VALIDATE_H
