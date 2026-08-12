#ifndef BHARAT_UAPI_MEMORY_HMEM_H
#define BHARAT_UAPI_MEMORY_HMEM_H

#include <stddef.h>
#include <stdint.h>

typedef uint64_t bharat_hmem_handle_t;

#define BH_HMEM_DESC_VERSION_1 UINT32_C(1)

#define BH_HMEM_DOMAIN_SYSTEM UINT32_C(0)
#define BH_HMEM_DOMAIN_DEVICE_LOCAL UINT32_C(1)
#define BH_HMEM_DOMAIN_SHARED UINT32_C(2)
#define BH_HMEM_DOMAIN_PERSISTENT UINT32_C(3)
#define BH_HMEM_DOMAIN_SECURE UINT32_C(4)

#define BH_HMEM_USAGE_CPU_READ (UINT64_C(1) << 0)
#define BH_HMEM_USAGE_CPU_WRITE (UINT64_C(1) << 1)
#define BH_HMEM_USAGE_DEVICE_READ (UINT64_C(1) << 2)
#define BH_HMEM_USAGE_DEVICE_WRITE (UINT64_C(1) << 3)
#define BH_HMEM_USAGE_DMA (UINT64_C(1) << 4)
#define BH_HMEM_USAGE_SHARED (UINT64_C(1) << 5)
#define BH_HMEM_USAGE_EXECUTE (UINT64_C(1) << 6)
#define BH_HMEM_USAGE_ALL ((UINT64_C(1) << 7) - 1)

#define BH_HMEM_PROP_CPU_COHERENT (UINT64_C(1) << 0)
#define BH_HMEM_PROP_DEVICE_COHERENT (UINT64_C(1) << 1)
#define BH_HMEM_PROP_CACHEABLE (UINT64_C(1) << 2)
#define BH_HMEM_PROP_PINNED (UINT64_C(1) << 3)
#define BH_HMEM_PROP_SECURE (UINT64_C(1) << 4)
#define BH_HMEM_PROP_ZERO_ON_ALLOC (UINT64_C(1) << 5)
#define BH_HMEM_PROP_ZERO_ON_FREE (UINT64_C(1) << 6)
#define BH_HMEM_PROP_ALL ((UINT64_C(1) << 7) - 1)

#define BH_HMEM_ACCESS_READ (UINT32_C(1) << 0)
#define BH_HMEM_ACCESS_WRITE (UINT32_C(1) << 1)
#define BH_HMEM_ACCESS_EXECUTE (UINT32_C(1) << 2)

#define BH_HMEM_RIGHT_READ (UINT64_C(1) << 0)
#define BH_HMEM_RIGHT_WRITE (UINT64_C(1) << 1)
#define BH_HMEM_RIGHT_MAP_CPU (UINT64_C(1) << 2)
#define BH_HMEM_RIGHT_MAP_DEVICE (UINT64_C(1) << 3)
#define BH_HMEM_RIGHT_SHARE (UINT64_C(1) << 4)
#define BH_HMEM_RIGHT_DERIVE (UINT64_C(1) << 5)
#define BH_HMEM_RIGHT_PIN (UINT64_C(1) << 6)
#define BH_HMEM_RIGHT_MIGRATE (UINT64_C(1) << 7)

typedef struct bharat_hmem_desc_v1 {
  uint32_t version;
  uint32_t struct_size;
  uint64_t size;
  uint64_t alignment;
  uint64_t usage_flags;
  uint64_t property_flags;
  uint32_t preferred_domain;
  uint32_t reserved0;
  uint64_t reserved[4];
} bharat_hmem_desc_v1_t;

typedef struct bharat_hmem_info_v1 {
  uint32_t version;
  uint32_t struct_size;
  uint64_t size;
  uint64_t alignment;
  uint64_t usage_flags;
  uint64_t property_flags;
  uint64_t content_generation;
  uint32_t preferred_domain;
  uint32_t cpu_map_count;
  uint64_t reserved[3];
} bharat_hmem_info_v1_t;

_Static_assert(sizeof(bharat_hmem_desc_v1_t) == 80U, "HMEM v1 descriptor ABI");
_Static_assert(offsetof(bharat_hmem_desc_v1_t, reserved) == 48U,
               "HMEM v1 reserved offset");
_Static_assert(sizeof(bharat_hmem_info_v1_t) == 80U, "HMEM v1 info ABI");

#endif
