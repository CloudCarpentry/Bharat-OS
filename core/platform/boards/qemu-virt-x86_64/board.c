#include "board/qemu-virt-x86_64/board.h"
#include "hal/hal.h"

void board_init(void) {
    hal_serial_init();
}
