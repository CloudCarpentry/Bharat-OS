#ifndef BHARAT_ISOLATION_H
#define BHARAT_ISOLATION_H

#include <stdint.h>

/*
 * Bharat-OS Isolation Classes
 * Defines authorization levels for privileged resources like MMIO, IRQs, and DMA.
 */

typedef enum {
    ISOLATION_CLASS_USER = 0,
    ISOLATION_CLASS_DRIVER = 1,
    ISOLATION_CLASS_SYSTEM = 2,
    ISOLATION_CLASS_ROOT = 3
} bharat_isolation_class_t;

#endif // BHARAT_ISOLATION_H
