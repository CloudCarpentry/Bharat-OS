#include <bharat/runtime/runtime.h>

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    bharat_runtime_log("\n========================================\n");
    bharat_runtime_log("  Hello from Bharat-OS!\n");
    bharat_runtime_log("========================================\n\n");

    return 0;
}
