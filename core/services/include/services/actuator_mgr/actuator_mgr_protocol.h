#pragma once
#include "drivers/actuator/actuator_device.h"

// IPC Message Definitions for Actuator Manager
#define ACTUATOR_MGR_IPC_OP_ARM 1
#define ACTUATOR_MGR_IPC_OP_DISARM 2
#define ACTUATOR_MGR_IPC_OP_SET_OUTPUT 3
#define ACTUATOR_MGR_IPC_OP_FORCE_SAFE_STATE 4
