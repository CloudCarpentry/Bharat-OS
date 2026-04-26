#ifndef BHARAT_CONFIG_H
#define BHARAT_CONFIG_H

/* AUTO-GENERATED VIA CMAKE configure_file(bharat_config.h.in -> generated/bharat_config.h). */
/* Edit bharat_config.h.in, not generated/bharat_config.h. */
/* Architecture Family */
/* #undef BHARAT_ARCH_X86 */
#define BHARAT_ARCH_ARM 1
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
/* #undef BHARAT_PROFILE_AUTOMOTIVE_ECU */
/* #undef BHARAT_PROFILE_AUTOMOTIVE_INFOTAINMENT */

/* Kernel Execution Profile */
/* #undef BHARAT_KERNEL_PROFILE_RT */
#define BHARAT_KERNEL_PROFILE_GP 1
/* #undef BHARAT_KERNEL_PROFILE_MIX */

/* IRQ Dispatch Mode */
#define BHARAT_IRQ_DISPATCH_GENERIC 1
/* #undef BHARAT_IRQ_DISPATCH_MIXED */
/* #undef BHARAT_IRQ_DISPATCH_RT */

/* Personality Profile */
/* #undef BHARAT_PERSONALITY_NATIVE */
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

/* Memory Capabilities */
#define BHARAT_ENABLE_MMU 1
/* #undef BHARAT_ENABLE_MPU */
#define BHARAT_ENABLE_ADVANCED_VM 1
#define BHARAT_ENABLE_IOMMU 1
#define BHARAT_ENABLE_DMA_MAP 1
#define BHARAT_ENABLE_COW 1
#define BHARAT_ENABLE_DEMAND_PAGING 1
#define BHARAT_ENABLE_SHARED_MEMORY 1
/* #undef BHARAT_ENABLE_NUMA */
#define BHARAT_ENABLE_ALLOC_CLASSES 1
#define BHARAT_ENABLE_MEMORY_STATS 1

#define MAX_SUPPORTED_CORES 32
#define BHARAT_CONFIG_CAP_REVOKE_MAX 256

/* Platform Firmware */
/* #undef BHARAT_PLATFORM_ACPI */
/* #undef BHARAT_PLATFORM_FDT */


/* Multikernel & URPC Configuration */
#define CONFIG_MULTIKERNEL 1
#define CONFIG_HETEROGENEOUS_CPU 1
#define CONFIG_URPC 1

#ifndef BHARAT_CACHE_LINE_SIZE
#define BHARAT_CACHE_LINE_SIZE 64
#endif

#ifndef BHARAT_ALIGNED_CACHE
#define BHARAT_ALIGNED_CACHE __attribute__((aligned(BHARAT_CACHE_LINE_SIZE)))
#endif

#endif /* BHARAT_CONFIG_H */
