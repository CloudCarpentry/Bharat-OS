#include "board/qemu-virt-riscv64/board.h"
#include "hal/hal.h"

void board_init(void) {
    hal_serial_init();
}
