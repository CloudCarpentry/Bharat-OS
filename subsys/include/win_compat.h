#ifndef BHARAT_WIN_COMPAT_H
#define BHARAT_WIN_COMPAT_H

#include "subsys.h"

/*
 * Bharat-OS Windows/NT Compatibility Layer
 * Requirements Mapping:
 * - Emulates PE format loading and imports.
 * - Simulates the NT kernel syscalls or acts as a Hardware Virtual Machine environment.
 */

int winnt_subsys_init(subsys_instance_t* env);

#endif // BHARAT_WIN_COMPAT_H
