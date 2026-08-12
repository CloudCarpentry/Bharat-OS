#include <bharat/os.h>
int main(void) { bh_system_info_t info; if (bh_system_info(&info) != BH_OK) return 1; return bh_console_write("Hello Bharat-OS\n") != BH_OK; }
