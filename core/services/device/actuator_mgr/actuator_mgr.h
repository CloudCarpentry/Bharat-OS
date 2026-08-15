#ifndef BHARAT_SERVICES_ACTUATOR_MGR_H
#define BHARAT_SERVICES_ACTUATOR_MGR_H

#include <bharat/uapi/device/actuator.h>
#include <bharat/uapi/syscall/bh_syscall_status.h>
#include "../../../drivers/actuator/virtual/virtual_motors.h"

typedef struct bh_actuator_mgr {
    bh_virtual_motor_bank_t *devices;
    uint8_t grants[BH_VIRTUAL_MOTOR_COUNT];
} bh_actuator_mgr_t;

void bh_actuator_mgr_init(bh_actuator_mgr_t *manager, bh_virtual_motor_bank_t *devices);
bh_status_t bh_actuator_mgr_open(bh_actuator_mgr_t *manager,
                             bh_actuator_type_t type,
                             uint32_t instance,
                             bh_actuator_handle_t *actuator);
bh_status_t bh_actuator_mgr_write(bh_actuator_mgr_t *manager,
                              bh_actuator_handle_t actuator,
                              const bh_actuator_command_t *command);
void bh_actuator_mgr_enter_safe_state(bh_actuator_mgr_t *manager);

#endif
