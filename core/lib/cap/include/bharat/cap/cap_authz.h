#ifndef BHARAT_CAP_AUTHZ_H
#define BHARAT_CAP_AUTHZ_H

#include <stdint.h>
#include <stdbool.h>
#include <bharat/cap/cap_validate.h>

#define BHARAT_MEM_CAP_NONE                 0
#define BHARAT_MEM_CAP_VIRT_ADDRSPACE       (1ULL << 0)
#define BHARAT_MEM_CAP_PAGE_MAP             (1ULL << 1)
#define BHARAT_MEM_CAP_PAGE_PROTECT         (1ULL << 2)
#define BHARAT_MEM_CAP_REGION_PROTECT       (1ULL << 3)
#define BHARAT_MEM_CAP_DEMAND_FAULT         (1ULL << 4)
#define BHARAT_MEM_CAP_SHARED_ASPACE        (1ULL << 5)
#define BHARAT_MEM_CAP_TLB_INVALIDATE       (1ULL << 6)
#define BHARAT_MEM_CAP_DMA_MAP              (1ULL << 7)
#define BHARAT_MEM_CAP_IOMMU                (1ULL << 8)
#define BHARAT_MEM_CAP_PER_CORE_PMM_CACHE   (1ULL << 9)

typedef enum {
    BHARAT_AUTHZ_SCOPE_ANY = 0,
    BHARAT_AUTHZ_SCOPE_OBJECT,
    BHARAT_AUTHZ_SCOPE_SERVICE,
} bharat_authz_scope_source_t;

typedef enum {
    BHARAT_AUTHZ_OBJ_SRC_ANY = 0,
    BHARAT_AUTHZ_OBJ_SRC_REQUEST,
} bharat_authz_object_source_t;

typedef enum {
    BHARAT_AUTHZ_PHASE_PRE_HANDLER = 0,
    BHARAT_AUTHZ_PHASE_POST_HANDLER,
} bharat_authz_phase_t;

typedef struct {
    uint32_t opcode;
    bharat_cap_object_type_t object_type;
    uint64_t required_rights;
    bharat_authz_scope_source_t scope_source;
    bharat_authz_object_source_t object_source;
    bharat_authz_phase_t validation_phase;
    uint64_t required_feature_cap;
} bharat_service_authz_desc_t;

int32_t bharat_service_dispatch_authorize(
    uint32_t service_id,
    uint32_t opcode,
    const bharat_service_authz_desc_t *descs,
    uint32_t desc_count,
    bharat_cap_handle_t caller_cap,
    uint64_t target_object_id
);

#endif // BHARAT_CAP_AUTHZ_H
