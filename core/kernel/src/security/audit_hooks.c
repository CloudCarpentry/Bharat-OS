#include <bharat/kernel/security/audit.h>
#include "console/console_core.h"
#include "lib/base/string.h"

#define KPRINT(s) console_write_raw(s, string_length(s))

void bh_audit_log_event(uint32_t event_id, const char *msg) {
    (void)event_id;
    // Minimal kernel hook: just log to console for now
    KPRINT("AUDIT EVENT\n");
    if (msg) KPRINT(msg);
    KPRINT("\n");
}

int bharat_audit_record(bharat_audit_event_type_t type,
                        uint32_t subject_id,
                        uint32_t object_id,
                        uint64_t val1,
                        uint64_t val2) {
    (void)type; (void)subject_id; (void)object_id; (void)val1; (void)val2;
    // Minimal kernel hook
    return 0;
}

int bharat_audit_init(void) {
    return 0;
}
