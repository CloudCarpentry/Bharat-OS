#ifndef BHARAT_CAPABILITY_H
#define BHARAT_CAPABILITY_H

#include <stdint.h>

#include "sched.h"

typedef enum {
    CAP_OBJ_NONE = 0,
    CAP_OBJ_ENDPOINT = 1,
    CAP_OBJ_MEMORY = 2,
    CAP_OBJ_SCHED = 3,
    CAP_OBJ_PROCESS = 4,
} cap_object_type_t;

typedef enum {
    CAP_PERM_NONE      = 0,
    CAP_PERM_SEND      = (1U << 0),
    CAP_PERM_RECEIVE   = (1U << 1),
    CAP_PERM_MAP       = (1U << 2),
    CAP_PERM_UNMAP     = (1U << 3),
    CAP_PERM_SCHEDULE  = (1U << 4),
    CAP_PERM_DELEGATE  = (1U << 5),
} cap_perm_t;

typedef struct {
    uint32_t id;
    cap_object_type_t type;
    uint32_t rights;
    uint64_t object_ref;
    uint8_t in_use;
} capability_entry_t;

typedef struct {
    capability_entry_t entries[64];
    uint32_t next_id;
} capability_table_t;

capability_table_t* cap_table_create(void);
int cap_table_init_for_process(kprocess_t* proc);
int cap_table_grant(capability_table_t* table,
                    cap_object_type_t type,
                    uint64_t object_ref,
                    uint32_t rights,
                    uint32_t* out_cap_id);
int cap_table_lookup(const capability_table_t* table,
                     uint32_t cap_id,
                     cap_object_type_t required_type,
                     uint32_t required_rights,
                     capability_entry_t* out_entry);
int cap_table_delegate(capability_table_t* src,
                       capability_table_t* dst,
                       uint32_t cap_id,
                       uint32_t delegated_rights,
                       uint32_t* out_new_cap_id);

#endif // BHARAT_CAPABILITY_H
