#include <bharat/uapi/system/footprint.h>

static struct bharat_footprint_stats g_footprint;

int bharat_footprint_read(struct bharat_footprint_stats *out) {
    if (!out) {
        return -1;
    }
    *out = g_footprint;
    return 0;
}
