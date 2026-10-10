#include <bharat/cap/cap.h>

bool bharat_cap_is_valid(bharat_cap_handle_t handle) {
    return handle != BHARAT_CAP_INVALID_HANDLE;
}

void bharat_cap_format(bharat_cap_handle_t handle, char *buf, uint32_t len) {
    if (!buf || len == 0) {
        return;
    }

    const char hex_chars[] = "0123456789abcdef";
    char temp[19];
    temp[0] = '0';
    temp[1] = 'x';

    bharat_cap_handle_t temp_handle = handle;
    for (int i = 0; i < 16; i++) {
        temp[17 - i] = hex_chars[temp_handle & 0xF];
        temp_handle >>= 4;
    }
    temp[18] = '\0';

    uint32_t i;
    for (i = 0; i < len - 1 && temp[i] != '\0'; i++) {
        buf[i] = temp[i];
    }
    buf[i] = '\0';
}

bharat_cap_rights_t bharat_cap_intersect_rights(bharat_cap_rights_t a, bharat_cap_rights_t b) {
    return a & b;
}
