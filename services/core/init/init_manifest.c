#include "init_manifest.h"
#include <stddef.h>

// Stubs for start functions
static int stub_start(void *ctx) {
    (void)ctx;
    return 0; // Success
}

static const init_service_id_t deps_namesvc[] = { INIT_SVC_NONE };
static const init_service_id_t deps_devmgr[] = { INIT_SVC_NAMESVC };
static const init_service_id_t deps_process_manager[] = { INIT_SVC_NAMESVC };
static const init_service_id_t deps_vm_manager[] = { INIT_SVC_NAMESVC, INIT_SVC_PROCESS_MANAGER };
static const init_service_id_t deps_servicemgr[] = { INIT_SVC_NAMESVC };
static const init_service_id_t deps_faultmgr[] = { INIT_SVC_NAMESVC };
static const init_service_id_t deps_boot_displayd[] = { INIT_SVC_NAMESVC };

const init_service_desc_t g_init_manifest[] = {
    {
        .id = INIT_SVC_NAMESVC,
        .name = "namesvc",
        .start_fn = stub_start,
        .probe_fn = NULL,
        .deps = deps_namesvc,
        .dep_count = 0,
        .retry_limit = 3,
        .policy = INIT_SERVICE_REQUIRED,
        .profile_mask = BHARAT_INIT_PROFILE_SMALL | BHARAT_INIT_PROFILE_EMBEDDED_RICH | BHARAT_INIT_PROFILE_MOBILE | BHARAT_INIT_PROFILE_DESKTOP | BHARAT_INIT_PROFILE_DRONE,
        .board_mask = BHARAT_INIT_BOARD_ANY,
        .personality_mask = BHARAT_INIT_PERSONALITY_ANY,
        .required_caps = BHARAT_INIT_CAP_NONE,
    },
    {
        .id = INIT_SVC_PROCESS_MANAGER,
        .name = "process_manager",
        .start_fn = stub_start,
        .probe_fn = NULL,
        .deps = deps_process_manager,
        .dep_count = 1,
        .retry_limit = 3,
        .policy = INIT_SERVICE_REQUIRED,
        .profile_mask = BHARAT_INIT_PROFILE_EMBEDDED_RICH | BHARAT_INIT_PROFILE_MOBILE | BHARAT_INIT_PROFILE_DESKTOP,
        .board_mask = BHARAT_INIT_BOARD_ANY,
        .personality_mask = BHARAT_INIT_PERSONALITY_ANY,
        .required_caps = BHARAT_INIT_CAP_NONE,
    },
    {
        .id = INIT_SVC_VM_MANAGER,
        .name = "vm_manager",
        .start_fn = stub_start,
        .probe_fn = NULL,
        .deps = deps_vm_manager,
        .dep_count = 2,
        .retry_limit = 3,
        .policy = INIT_SERVICE_OPTIONAL,
        .profile_mask = BHARAT_INIT_PROFILE_EMBEDDED_RICH | BHARAT_INIT_PROFILE_MOBILE | BHARAT_INIT_PROFILE_DESKTOP,
        .board_mask = BHARAT_INIT_BOARD_ANY,
        .personality_mask = BHARAT_INIT_PERSONALITY_ANY,
        .required_caps = BHARAT_INIT_CAP_MMU,
    },
    {
        .id = INIT_SVC_DEVMGR,
        .name = "devmgr",
        .start_fn = stub_start,
        .probe_fn = NULL,
        .deps = deps_devmgr,
        .dep_count = 1,
        .retry_limit = 2,
        .policy = INIT_SERVICE_OPTIONAL,
        .profile_mask = BHARAT_INIT_PROFILE_SMALL | BHARAT_INIT_PROFILE_EMBEDDED_RICH | BHARAT_INIT_PROFILE_MOBILE | BHARAT_INIT_PROFILE_DESKTOP,
        .board_mask = BHARAT_INIT_BOARD_ANY,
        .personality_mask = BHARAT_INIT_PERSONALITY_ANY,
        .required_caps = BHARAT_INIT_CAP_NONE,
    },
    {
        .id = INIT_SVC_FAULTMGR,
        .name = "faultmgr",
        .start_fn = stub_start,
        .probe_fn = NULL,
        .deps = deps_faultmgr,
        .dep_count = 1,
        .retry_limit = 1,
        .policy = INIT_SERVICE_OPTIONAL,
        .profile_mask = BHARAT_INIT_PROFILE_SMALL | BHARAT_INIT_PROFILE_EMBEDDED_RICH | BHARAT_INIT_PROFILE_DRONE,
        .board_mask = BHARAT_INIT_BOARD_ANY,
        .personality_mask = BHARAT_INIT_PERSONALITY_ANY,
        .required_caps = BHARAT_INIT_CAP_NONE,
    },
    {
        .id = INIT_SVC_SERVICEMGR,
        .name = "servicemgr",
        .start_fn = stub_start,
        .probe_fn = NULL,
        .deps = deps_servicemgr,
        .dep_count = 1,
        .retry_limit = 3,
        .policy = INIT_SERVICE_OPTIONAL,
        .profile_mask = BHARAT_INIT_PROFILE_EMBEDDED_RICH | BHARAT_INIT_PROFILE_MOBILE | BHARAT_INIT_PROFILE_DESKTOP,
        .board_mask = BHARAT_INIT_BOARD_ANY,
        .personality_mask = BHARAT_INIT_PERSONALITY_ANY,
        .required_caps = BHARAT_INIT_CAP_NONE,
    },
    {
        .id = INIT_SVC_BOOT_DISPLAYD,
        .name = "boot_displayd",
        .start_fn = stub_start,
        .probe_fn = NULL,
        .deps = deps_boot_displayd,
        .dep_count = 1,
        .retry_limit = 2,
        .policy = INIT_SERVICE_OPTIONAL,
        .profile_mask = BHARAT_INIT_PROFILE_MOBILE | BHARAT_INIT_PROFILE_DESKTOP,
        .board_mask = BHARAT_INIT_BOARD_ANY,
        .personality_mask = BHARAT_INIT_PERSONALITY_ANY,
        .required_caps = BHARAT_INIT_CAP_DISPLAY,
    },
    {
        .id = INIT_SVC_APP_PAYLOAD,
        .name = "app_payload",
        .start_fn = stub_start,
        .probe_fn = NULL,
        .deps = NULL,
        .dep_count = 0,
        .retry_limit = 0,
        .policy = INIT_SERVICE_OPTIONAL,
        .profile_mask = BHARAT_INIT_PROFILE_TINY,
        .board_mask = BHARAT_INIT_BOARD_ANY,
        .personality_mask = BHARAT_INIT_PERSONALITY_ANY,
        .required_caps = BHARAT_INIT_CAP_NONE,
    }
};

const size_t g_init_manifest_count = sizeof(g_init_manifest) / sizeof(g_init_manifest[0]);
