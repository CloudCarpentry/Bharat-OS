#pragma once
#include <stdint.h>
#include <bharat/uapi/system/constraints.h>

int sys_thread_set_constraints(int tid,
                               const bh_exec_constraints_t *c);
