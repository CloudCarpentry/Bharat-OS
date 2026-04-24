#include "test_framework.h"

// Basic implementation using bharat_write
extern int bharat_write(int fd, const void* buf, unsigned long count);

static int string_length(const char* s) {
    int len = 0;
    while (s[len]) {
        len++;
    }
    return len;
}

void console_print(const char* str) {
    bharat_write(1, str, string_length(str)); // stdout
}

static void print_hex(uint64_t val) {
    char buf[19];
    int i = 15;
    buf[16] = '\n';
    buf[17] = '\0';
    if (val == 0) {
        console_print("0x0");
        return;
    }
    while (i >= 0 && val > 0) {
        int nibble = val & 0xF;
        if (nibble < 10) {
            buf[i] = '0' + nibble;
        } else {
            buf[i] = 'a' + (nibble - 10);
        }
        val >>= 4;
        i--;
    }
    console_print("0x");
    console_print(&buf[i + 1]);
}

void console_print_metric(const char* test_name, const char* metric_name, uint64_t val) {
    console_print("[METRIC] test=");
    console_print(test_name);
    console_print(" metric=");
    console_print(metric_name);
    console_print(" value=");
    print_hex(val);
    console_print("\n");
}
