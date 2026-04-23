#pragma once
#include <stdint.h>
#include <bharat/uapi/system/constraints.h>

int sys_thread_set_constraints(int tid,
                               const bh_exec_constraints_t *c);
int sys_thread_get_constraints(int tid,
                               bh_exec_constraints_t *out_c);
