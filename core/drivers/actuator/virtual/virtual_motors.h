#ifndef BHARAT_DRIVERS_VIRTUAL_MOTORS_H
#define BHARAT_DRIVERS_VIRTUAL_MOTORS_H

#include <bharat/uapi/device/actuator.h>
#include <bharat/uapi/syscall/bh_syscall_status.h>

#define BH_VIRTUAL_MOTOR_COUNT 4u

typedef struct bh_virtual_motor {
    float output;
    uint64_t last_request_id;
    uint8_t armed;
} bh_virtual_motor_t;

typedef struct bh_virtual_motor_bank {
    bh_virtual_motor_t motors[BH_VIRTUAL_MOTOR_COUNT];
} bh_virtual_motor_bank_t;

void bh_virtual_motors_init(bh_virtual_motor_bank_t *bank);
bh_status_t bh_virtual_motor_write(bh_virtual_motor_bank_t *bank,
                                   uint32_t instance,
                                   const bh_actuator_command_t *command);
bh_status_t bh_virtual_motor_safe(bh_virtual_motor_bank_t *bank, uint32_t instance);

#endif
