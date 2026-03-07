#ifndef BHARAT_CONFIG_H
#define BHARAT_CONFIG_H

/* Architecture Family */
#define BHARAT_ARCH_X86 1
/* #undef BHARAT_ARCH_ARM */
/* #undef BHARAT_ARCH_RISCV */

/* Architecture Bitness */
/* #undef BHARAT_ARCH_32BIT */
#define BHARAT_ARCH_64BIT 1

/* Architecture Variant */
#define BHARAT_ARCH_VARIANT_GENERIC 1
/* #undef BHARAT_ARCH_VARIANT_SHAKTI */

/* Device Profile */
#define BHARAT_PROFILE_DESKTOP 1
/* #undef BHARAT_PROFILE_MOBILE */
/* #undef BHARAT_PROFILE_EDGE */
/* #undef BHARAT_PROFILE_DATACENTER */
/* #undef BHARAT_PROFILE_NETWORK_APPLIANCE */
/* #undef BHARAT_PROFILE_DRONE */
/* #undef BHARAT_PROFILE_ROBOT */
/* #undef BHARAT_PROFILE_RTOS */

/* Personality Profile */
/* #undef BHARAT_PERSONALITY_NONE */
#define BHARAT_PERSONALITY_LINUX 1
/* #undef BHARAT_PERSONALITY_WINDOWS */
/* #undef BHARAT_PERSONALITY_MAC */

/* ISA Baseline */
/* #undef BHARAT_ISA_BASELINE_X86_64_V2 */
/* #undef BHARAT_ISA_BASELINE_RV64GC */
/* ... add more baselines as needed ... */

/* ISA Features (Dynamic based on CMake lists) */
/* The build system will pass these as compile definitions */
/* For example: -DBHARAT_ISA_FEATURE_AVX2=1 */

/* Hardware Accelerators (Dynamic based on CMake lists) */
/* For example: -DBHARAT_ACCEL_GPU=1 */

/* Platform Firmware */
#define BHARAT_PLATFORM_ACPI 1
/* #undef BHARAT_PLATFORM_FDT */

#endif /* BHARAT_CONFIG_H */
