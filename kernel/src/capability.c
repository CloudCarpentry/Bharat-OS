#include "capability.h"
#include "kernel_safety.h"

// @cite seL4: Formal Verification of an OS Kernel (Klein et al., 2009)
// seL4 capability model and verification-oriented discipline
#include <stddef.h>

#define MAX_CAP_TABLES 32U

static capability_table_t g_cap_tables[MAX_CAP_TABLES];
static uint8_t g_cap_tables_used[MAX_CAP_TABLES];


/*@
  assigns \nothing;
  ensures \result == 0 || \result == 1;
*/
static int cap_rights_valid(cap_object_type_t type, uint32_t rights) {
    switch (type) {
    case CAP_OBJ_ENDPOINT:
        return (rights & ~(CAP_PERM_SEND | CAP_PERM_RECEIVE | CAP_PERM_DELEGATE)) == 0U;
    case CAP_OBJ_MEMORY:
        return (rights & ~(CAP_PERM_MAP | CAP_PERM_UNMAP | CAP_PERM_DELEGATE)) == 0U;
    case CAP_OBJ_SCHED:
        return (rights & ~(CAP_PERM_SCHEDULE | CAP_PERM_DELEGATE)) == 0U;
    case CAP_OBJ_PROCESS:
        return (rights & ~(CAP_PERM_DELEGATE)) == 0U;
    default:
        return 0;
    }
}

/*@
  requires table != \null;
  requires \valid(table);
  assigns \nothing;
  ensures \result == \null || \valid(\result);
*/
static capability_entry_t* cap_find_entry(capability_table_t* table, uint32_t cap_id) {
    if (!BHARAT_PTR_NON_NULL(table) || cap_id == 0U) {
        return NULL;
    }

    /*@
      loop invariant 0 <= i <= BHARAT_ARRAY_SIZE(table->entries);
      loop assigns i;
      loop variant BHARAT_ARRAY_SIZE(table->entries) - i;
    */
    for (size_t i = 0; i < BHARAT_ARRAY_SIZE(table->entries); ++i) {
        capability_entry_t* e = &table->entries[i];
        if (e->in_use != 0U && e->id == cap_id) {
            return e;
        }
    }
    return NULL;
}

capability_table_t* cap_table_create(void) {
    /*@
      loop invariant 0 <= i <= BHARAT_ARRAY_SIZE(g_cap_tables);
      loop assigns i, g_cap_tables_used[0..MAX_CAP_TABLES-1], g_cap_tables[0..MAX_CAP_TABLES-1];
      loop variant BHARAT_ARRAY_SIZE(g_cap_tables) - i;
    */
    for (size_t i = 0; i < BHARAT_ARRAY_SIZE(g_cap_tables); ++i) {
        if (g_cap_tables_used[i] == 0U) {
            g_cap_tables_used[i] = 1U;
            capability_table_t* t = &g_cap_tables[i];
            t->next_id = 1U;
            /*@
              loop invariant 0 <= j <= BHARAT_ARRAY_SIZE(t->entries);
              loop assigns j, t->entries[0..BHARAT_ARRAY_SIZE(t->entries)-1].in_use;
              loop variant BHARAT_ARRAY_SIZE(t->entries) - j;
            */
            for (size_t j = 0; j < BHARAT_ARRAY_SIZE(t->entries); ++j) {
                t->entries[j].in_use = 0U;
            }
            return t;
        }
    }

    return NULL;
}

int cap_table_init_for_process(kprocess_t* proc) {
    if (!BHARAT_PTR_NON_NULL(proc)) {
        return -1;
    }

    capability_table_t* t = cap_table_create();
    if (!t) {
        return -2;
    }

    proc->security_sandbox_ctx = t;
    return 0;
}

int cap_table_grant(capability_table_t* table,
                    cap_object_type_t type,
                    uint64_t object_ref,
                    uint32_t rights,
                    uint32_t* out_cap_id) {
    if (!BHARAT_PTR_NON_NULL(table) || type == CAP_OBJ_NONE) {
        return -1;
    }

    if (!cap_rights_valid(type, rights) || rights == CAP_PERM_NONE) {
        return -3;
    }

    uint32_t found_id = 0;
    int ret = -2;

    /*@
      loop invariant 0 <= i <= BHARAT_ARRAY_SIZE(table->entries);
      loop assigns i, table->entries[0..BHARAT_ARRAY_SIZE(table->entries)-1], table->next_id, found_id, ret;
      loop variant BHARAT_ARRAY_SIZE(table->entries) - i;
    */
    for (size_t i = 0; i < BHARAT_ARRAY_SIZE(table->entries); ++i) {
        capability_entry_t* e = &table->entries[i];
        if (e->in_use == 0U) {
            e->in_use = 1U;
            e->id = table->next_id++;
            e->type = type;
            e->rights = rights;
            e->object_ref = object_ref;
            found_id = e->id;
            ret = 0;
            break;
        }
    }

    if (ret == 0 && out_cap_id) {
        *out_cap_id = found_id;
    }

    return ret;
}

int cap_table_lookup(const capability_table_t* table,
                     uint32_t cap_id,
                     cap_object_type_t required_type,
                     uint32_t required_rights,
                     capability_entry_t* out_entry) {
    if (!BHARAT_PTR_NON_NULL(table) || cap_id == 0U) {
        return -1;
    }

    capability_entry_t found_entry = {0};
    int ret = -4;

    /*@
      loop invariant 0 <= i <= BHARAT_ARRAY_SIZE(table->entries);
      loop assigns i, found_entry, ret;
      loop variant BHARAT_ARRAY_SIZE(table->entries) - i;
    */
    for (size_t i = 0; i < BHARAT_ARRAY_SIZE(table->entries); ++i) {
        const capability_entry_t* e = &table->entries[i];
        if (e->in_use != 0U && e->id == cap_id) {
            if (required_type != CAP_OBJ_NONE && e->type != required_type) {
                ret = -2;
                break;
            }
            if ((e->rights & required_rights) != required_rights) {
                ret = -3;
                break;
            }
            found_entry = *e;
            ret = 0;
            break;
        }
    }

    if (ret == 0 && out_entry) {
        *out_entry = found_entry;
    }

    return ret;
}

int cap_table_delegate(capability_table_t* src,
                       capability_table_t* dst,
                       uint32_t cap_id,
                       uint32_t delegated_rights,
                       uint32_t* out_new_cap_id) {
    if (!BHARAT_PTR_NON_NULL(src) || !BHARAT_PTR_NON_NULL(dst) || cap_id == 0U) {
        return -1;
    }

    capability_entry_t* src_entry = cap_find_entry(src, cap_id);
    if (!src_entry) {
        return -2;
    }

    if ((src_entry->rights & CAP_PERM_DELEGATE) == 0U) {
        return -3;
    }

    if ((src_entry->rights & delegated_rights) != delegated_rights) {
        return -4;
    }

    if (!cap_rights_valid(src_entry->type, delegated_rights) || delegated_rights == CAP_PERM_NONE) {
        return -5;
    }

    return cap_table_grant(dst, src_entry->type, src_entry->object_ref, delegated_rights, out_new_cap_id);
}
