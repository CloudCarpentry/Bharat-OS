#ifndef BHARAT_PROFILES_DEVICE_POLICY_H
#define BHARAT_PROFILES_DEVICE_POLICY_H

#include "bharat_config.h"

/*
 * Device Profile Policy Headers
 * Controls scheduler defaults, power policies, and included drivers
 */

#if defined(BHARAT_PROFILE_DESKTOP)
#define DEVICE_PROFILE_NAME "Desktop"
#define SCHEDULER_DEFAULT_POLICY "Fair"
#define POWER_POLICY "Performance"

#elif defined(BHARAT_PROFILE_MOBILE)
#define DEVICE_PROFILE_NAME "Mobile"
#define SCHEDULER_DEFAULT_POLICY "Energy_Aware"
#define POWER_POLICY "Battery_Saver"

#elif defined(BHARAT_PROFILE_EDGE)
#define DEVICE_PROFILE_NAME "Edge"
#define SCHEDULER_DEFAULT_POLICY "Realtime_Mixed"
#define POWER_POLICY "Balanced"

#elif defined(BHARAT_PROFILE_DATACENTER)
#define DEVICE_PROFILE_NAME "Datacenter"
#define SCHEDULER_DEFAULT_POLICY "Throughput"
#define POWER_POLICY "Max_Performance"

#elif defined(BHARAT_PROFILE_NETWORK_APPLIANCE)
#define DEVICE_PROFILE_NAME "Network Appliance"
#define SCHEDULER_DEFAULT_POLICY "Realtime"
#define POWER_POLICY "Performance"

#elif defined(BHARAT_PROFILE_DRONE) || defined(BHARAT_PROFILE_ROBOT) || defined(BHARAT_PROFILE_RTOS)
#define DEVICE_PROFILE_NAME "RTOS / Robotics"
#define SCHEDULER_DEFAULT_POLICY "Strict_Realtime"
#define POWER_POLICY "Mission_Critical"

#else
#define DEVICE_PROFILE_NAME "Unknown"
#define SCHEDULER_DEFAULT_POLICY "Default"
#define POWER_POLICY "Default"
#endif

#endif /* BHARAT_PROFILES_DEVICE_POLICY_H */
