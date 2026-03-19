#include <bharat/cap/cap.h>

bool bharat_cap_is_valid(bharat_cap_handle_t handle) {
    return handle != BHARAT_CAP_INVALID_HANDLE;
}

void bharat_cap_format(bharat_cap_handle_t handle, char *buf, uint32_t len) {
    // Stub implementation
    if (buf && len > 0) {
        buf[0] = '\0';
    }
}

bharat_cap_rights_t bharat_cap_intersect_rights(bharat_cap_rights_t a, bharat_cap_rights_t b) {
    return a & b;
}
