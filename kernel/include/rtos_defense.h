#ifndef BHARAT_RTOS_DEFENSE_H
#define BHARAT_RTOS_DEFENSE_H

#include <stdint.h>
#include "sched.h"

/*
 * Bharat-OS Hard Real-Time & Defense Scheduler Extensions
 * Provides deterministic fault-tolerant scheduling for mission-critical systems
 * like aerospace, autonomous drones, and military defense embedded applications.
 */

// Fault tolerance models
typedef enum {
    FT_MODE_NONE = 0,
    FT_MODE_DUAL_MODULAR_REDUNDANCY = 2, // Lock-step execution across 2 physical cores
    FT_MODE_TRIPLE_MODULAR_REDUNDANCY = 3 // 2-out-of-3 voting for radiation hardening
} fault_tolerance_mode_t;

// Real-Time constraint specification
typedef struct {
    uint32_t deadline_us; // Strict deadline in microseconds
    uint32_t period_us;
    uint32_t wcet_us; // Worst Case Execution Time
    
    fault_tolerance_mode_t ft_mode;
    
    // Hardware Watchdog to trip if deadline is missed
    int enable_hard_watchdog;
} rtos_constraints_t;

// Elevate a standard kernel thread to a defense-grade Hard RTOS thread
int rtos_elevate_thread(kthread_t* thread, rtos_constraints_t* constraints);

// Pet the hardware watchdog (called periodically by the RTOS task)
void rtos_watchdog_pet(void);

// Register a Fault Handler for Modular Redundancy mismatch or missed deadlines
void rtos_register_fault_handler(void (*handler)(kthread_t* failed_thread));

#endif // BHARAT_RTOS_DEFENSE_H
