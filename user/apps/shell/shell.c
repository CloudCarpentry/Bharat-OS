#include <stdint.h>
#include <stddef.h>

extern int bharat_write(int fd, const void* buf, size_t count);
extern int bharat_read(int fd, void* buf, size_t count);
// Depending on how testing is structured, it might need to fork/exec app_test,
// or we can build the tests natively into the shell if it's simpler for now.

static int string_length(const char* s) {
    int len = 0;
    while (s[len]) {
        len++;
    }
    return len;
}

static void console_print(const char* str) {
    bharat_write(1, str, string_length(str)); // stdout
}

static int my_strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}

static void my_memset(void* ptr, int value, size_t num) {
    unsigned char* p = (unsigned char*)ptr;
    for (size_t i = 0; i < num; i++) {
        p[i] = (unsigned char)value;
    }
}

static void run_command(const char* cmd) {
    if (my_strcmp(cmd, "help") == 0) {
        console_print("Bharat-OS Shell Commands:\n");
        console_print("  help        - Show this message\n");
        console_print("  run all     - Run all tests\n");
        console_print("  run pmm     - Run PMM tests\n");
        console_print("  run vmm     - Run VMM tests\n");
        console_print("  stats       - Show system statistics\n");
    } else if (my_strcmp(cmd, "run all") == 0) {
        console_print("[SHELL] Starting app_test...\n");
        // In a real shell with execve:
        // execve("app_test", ...);
        // For now, print a placeholder message.
        console_print("To run tests, execute 'app_test' via init process or test harness.\n");
    } else if (my_strcmp(cmd, "stats") == 0) {
         console_print("System Stats: Uptime=Unknown Memory=Unknown\n");
    } else if (string_length(cmd) > 0) {
        console_print("Unknown command: ");
        console_print(cmd);
        console_print("\n");
    }
}

int main(void) {
    char buf[128];
    console_print("\nWelcome to Bharat-OS Interactive Shell\n");

    while (1) {
        console_print("bharat> ");
        my_memset(buf, 0, sizeof(buf));

        // Very basic read loop - assuming terminal sends newline
        int len = 0;
        char c;
        while (len < (int)sizeof(buf) - 1) {
            if (bharat_read(0, &c, 1) == 1) {
                if (c == '\n' || c == '\r') {
                    console_print("\n");
                    break;
                } else if (c == '\b' || c == 0x7F) { // Backspace
                    if (len > 0) {
                        len--;
                        console_print("\b \b");
                    }
                } else {
                    buf[len++] = c;
                    bharat_write(1, &c, 1);
                }
            } else {
                // Read failed or no data, depending on blocking semantics
                // Delay a bit
                volatile int d = 10000;
                while (d-- > 0);
            }
        }
        buf[len] = '\0';
        run_command(buf);
    }
    return 0;
}
