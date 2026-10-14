#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <bharat/uapi/namesvc/contract.h>
#include <bharat/cap/cap.h>

#define NAMESVC_MAX_ENTRIES 256
#define NAMESVC_STATUS_OK 0
#define NAMESVC_STATUS_ERR_NOTFOUND -1
#define NAMESVC_STATUS_ERR_NOSPACE -2
#define NAMESVC_STATUS_ERR_EXISTS -3
#define NAMESVC_STATUS_ERR_INVAL -4
#define NAMESVC_STATUS_ERR_PERM -5

extern bool g_force_stale;

typedef struct {
    char name[NAMESVC_MAX_NAME_LEN];
    bharat_cap_handle_t endpoint;
    bharat_service_id_t service_id;
    uint32_t version;
    uint32_t flags;
    uint64_t owner_id;
    bool in_use;
} namesvc_entry_t;

void namesvc_registry_init(void);

int32_t namesvc_registry_add(const char *service_name,
                             bharat_service_id_t sid,
                             uint32_t version,
                             uint32_t flags,
                             bharat_cap_handle_t endpoint);

int32_t namesvc_registry_remove(const char *service_name, bharat_cap_handle_t caller_cap);

int32_t namesvc_registry_lookup(const char *service_name,
                                uint32_t requested_version,
                                bool exact_version,
                                bharat_cap_handle_t *out_endpoint,
                                bharat_service_id_t *out_sid,
                                uint32_t *out_version,
                                uint32_t *out_flags);
