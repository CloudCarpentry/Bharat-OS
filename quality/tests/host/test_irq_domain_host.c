#include <stdio.h>

extern int boot_test_irq_domain(void);

void hal_serial_write(const char *str) {
    printf("%s", str);
}

int main() {
    printf("Running IRQ Domain Tests...\n");
    int res = boot_test_irq_domain();
    if (res == 0) {
        printf("Tests passed.\n");
    } else {
        printf("Tests failed.\n");
    }
    return res;
}
