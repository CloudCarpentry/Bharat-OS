#ifndef BHARAT_INIT_MANIFEST_H
#define BHARAT_INIT_MANIFEST_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef enum {
    BHARAT_INIT_PROFILE_TINY = 1,
    BHARAT_INIT_PROFILE_SMALL = 2,
    BHARAT_INIT_PROFILE_EMBEDDED_RICH = 4,
    BHARAT_INIT_PROFILE_MOBILE = 8,
    BHARAT_INIT_PROFILE_DESKTOP = 16,
    BHARAT_INIT_PROFILE_DRONE = 32,
} bharat_init_profile_t;

typedef enum {
    INIT_SERVICE_DISABLED = 0,
    INIT_SERVICE_OPTIONAL,
    INIT_SERVICE_REQUIRED,
} init_service_policy_t;

typedef enum {
    INIT_SERVICE_STOPPED = 0,
    INIT_SERVICE_STARTING,
    INIT_SERVICE_RUNNING,
    INIT_SERVICE_FAILED,
    INIT_SERVICE_SKIPPED,
} init_service_state_t;

typedef uint64_t bharat_init_profile_mask_t;
typedef uint64_t bharat_init_cap_mask_t;
typedef uint64_t bharat_init_board_mask_t;
typedef uint64_t bharat_init_personality_mask_t;

// Capability bitmask flags
#define BHARAT_INIT_CAP_NONE            (0)
#define BHARAT_INIT_CAP_NETWORK         (1 << 0)
#define BHARAT_INIT_CAP_STORAGE         (1 << 1)
#define BHARAT_INIT_CAP_DISPLAY         (1 << 2)
#define BHARAT_INIT_CAP_SENSORS         (1 << 3)
#define BHARAT_INIT_CAP_MMU             (1 << 4)

// Platform mask placeholders (example)
#define BHARAT_INIT_BOARD_ANY           (0xFFFFFFFFU)
#define BHARAT_INIT_PERSONALITY_ANY     (0xFFFFFFFFU)

// Service IDs
typedef enum {
    INIT_SVC_NONE = 0,
    INIT_SVC_NAMESVC = 1,
    INIT_SVC_PROCESS_MANAGER = 2,
    INIT_SVC_VM_MANAGER = 3,
    INIT_SVC_DEVMGR = 4,
    INIT_SVC_FAULTMGR = 5,
    INIT_SVC_SERVICEMGR = 6,
    INIT_SVC_BOOT_DISPLAYD = 7,
    INIT_SVC_TELEMETRYMGR = 8,
    INIT_SVC_STORAGEMGR = 9,
    INIT_SVC_ACCELMGR = 10,
    INIT_SVC_APP_PAYLOAD = 11,
    // Add more as needed
} init_service_id_t;

typedef struct {
    init_service_id_t id;
    const char *name;
    int (*start_fn)(void *ctx);
    int (*probe_fn)(void *ctx);      // optional capability/platform precheck
    const init_service_id_t *deps;
    uint8_t dep_count;
    uint8_t retry_limit;
    init_service_policy_t policy;
    bharat_init_profile_mask_t profile_mask;
    bharat_init_board_mask_t board_mask;
    bharat_init_personality_mask_t personality_mask;
    bharat_init_cap_mask_t required_caps;
} init_service_desc_t;

// Generic manifest declaration
extern const init_service_desc_t g_init_manifest[];
extern const size_t g_init_manifest_count;

#endif // BHARAT_INIT_MANIFEST_H
