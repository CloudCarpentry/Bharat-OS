#include "policymgr.h"

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    policymgr_init();
    policymgr_run();

    return 0;
}
