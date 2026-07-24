#include <stdio.h>

extern void diag_telemetry_init(void);

int main(void) {
    printf("Diag Service started.\n");

    // Initialize the telemetry subsystem
    diag_telemetry_init();

    return 0;
}
