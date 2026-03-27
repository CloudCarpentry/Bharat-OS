#include <stdint.h>
#include <stddef.h>

extern int bharat_write(int fd, const void* buf, size_t count);
extern void bharat_exit(int status);

// Minimal print for demo
void print(const char *msg) {
    int len = 0;
    while(msg[len]) { len++; }
    bharat_write(1, msg, len);
}

// The entry point for the sample app
void _start() {
    print("Hello from Bharat-OS SDK Sample App!\n");
    bharat_exit(0);
}
