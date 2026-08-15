#ifndef BHARAT_SDK_CAPABILITY_H
#define BHARAT_SDK_CAPABILITY_H
#include <bharat/types.h>
typedef struct bh_cap_info { uint32_t struct_size; uint32_t object_type; uint64_t rights; uint32_t generation; uint32_t owner; } bh_cap_info_t;
bh_status_t bh_cap_query(bh_cap_t capability, bh_cap_info_t *info);
bh_status_t bh_cap_duplicate(bh_cap_t capability, uint64_t rights, bh_cap_t *duplicate);
bh_status_t bh_cap_close(bh_cap_t capability);
#endif
