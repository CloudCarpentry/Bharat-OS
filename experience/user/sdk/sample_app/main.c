#include <stdint.h>
#include <stddef.h>

extern int bharat_write(int fd, const void* buf, size_t count);

// Minimal print for demo
void print(const char *msg) {
    int len = 0;
    while(msg[len]) { len++; }
    bharat_write(1, msg, len);
}

int main() {
    print("Hello from Bharat-OS SDK Sample App!\n");
    return 0;
}
