#include "subsystem_profile.h"

#if defined(__has_include)
#  if __has_include("bharat_config.h")
#    include "bharat_config.h"
#  endif
#endif

static bharat_storage_profile_t g_storage_profile;
static bharat_network_profile_t g_network_profile;
static int g_subsystems_ready;

static int streq(const char *a, const char *b) {
    if (!a || !b) {
        return 0;
    }

    while (*a != '\0' && *b != '\0') {
        if (*a != *b) {
            return 0;
        }
        ++a;
        ++b;
    }

    return *a == *b;
}

static void set_compile_time_storage_defaults(void) {
    uint32_t features = BHARAT_STORAGE_BLOCK_LAYER | BHARAT_STORAGE_RAMDISK;

#if defined(BHARAT_PROFILE_DESKTOP) || defined(BHARAT_PROFILE_DATACENTER) || defined(BHARAT_PROFILE_NETWORK_APPLIANCE)
    features |= BHARAT_STORAGE_NVME;
#endif

#if defined(BHARAT_ARCH_X86)
    features |= BHARAT_STORAGE_AHCI_SATA;
#endif

#if defined(BHARAT_PROFILE_MOBILE) || defined(BHARAT_PROFILE_EDGE) || defined(BHARAT_PROFILE_RTOS)
    features |= BHARAT_STORAGE_EMMC_SD;
#endif

#if defined(BHARAT_PROFILE_MOBILE) || defined(BHARAT_PROFILE_EDGE) || defined(BHARAT_PROFILE_DRONE) || defined(BHARAT_PROFILE_ROBOT) || defined(BHARAT_PROFILE_RTOS) || defined(BHARAT_PROFILE_AUTOMOTIVE_ECU)
    features |= BHARAT_STORAGE_FLASH_MTD;
#endif

    g_storage_profile.features = features;
}

static void set_compile_time_network_defaults(void) {
    uint32_t features = BHARAT_NET_ETHERNET;

#if defined(BHARAT_PROFILE_DESKTOP) || defined(BHARAT_PROFILE_DATACENTER) || defined(BHARAT_PROFILE_NETWORK_APPLIANCE)
    features |= BHARAT_NET_FULL_TCPIP_STACK;
#else
    features |= BHARAT_NET_LIGHTWEIGHT_STACK;
#endif

#if defined(BHARAT_PROFILE_MOBILE) || defined(BHARAT_PROFILE_AUTOMOTIVE_INFOTAINMENT)
    features |= BHARAT_NET_WIFI;
#endif

#if defined(BHARAT_PROFILE_DATACENTER) || defined(BHARAT_PROFILE_NETWORK_APPLIANCE)
    features |= BHARAT_NET_ZERO_COPY_PATH;
#endif

#if defined(BHARAT_PROFILE_DATACENTER)
    features |= BHARAT_NET_VIRTIO;
#endif

#if defined(BHARAT_PROFILE_AUTOMOTIVE_ECU) || defined(BHARAT_PROFILE_RTOS)
    features |= BHARAT_NET_TSN_EXT | BHARAT_NET_CAN_EXT | BHARAT_NET_ETHERCAT_EXT;
#endif

#if defined(BHARAT_PERSONALITY_LINUX) || defined(BHARAT_PERSONALITY_WINDOWS) || defined(BHARAT_PERSONALITY_MAC)
    features |= BHARAT_NET_FULL_TCPIP_STACK;
    features &= ~BHARAT_NET_LIGHTWEIGHT_STACK;
#endif

    g_network_profile.features = features;
}

void bharat_subsystems_init(const char *boot_hw_profile) {
    set_compile_time_storage_defaults();
    set_compile_time_network_defaults();

    if (streq(boot_hw_profile, "vm")) {
        g_storage_profile.features |= BHARAT_STORAGE_RAMDISK;
        g_storage_profile.features &= ~BHARAT_STORAGE_AHCI_SATA;

        g_network_profile.features |= BHARAT_NET_VIRTIO | BHARAT_NET_FULL_TCPIP_STACK;
        g_network_profile.features &= ~BHARAT_NET_LIGHTWEIGHT_STACK;
    } else if (streq(boot_hw_profile, "mobile")) {
        g_storage_profile.features |= BHARAT_STORAGE_EMMC_SD | BHARAT_STORAGE_FLASH_MTD;
        g_network_profile.features |= BHARAT_NET_WIFI | BHARAT_NET_LIGHTWEIGHT_STACK;
    } else if (streq(boot_hw_profile, "network_appliance")) {
        g_network_profile.features |= BHARAT_NET_ZERO_COPY_PATH | BHARAT_NET_ETHERNET;
        g_network_profile.features &= ~BHARAT_NET_WIFI;
    }

    g_subsystems_ready = 1;
}

int bharat_subsystems_ready(void) {
    return g_subsystems_ready;
}

const bharat_storage_profile_t *bharat_storage_active_profile(void) {
    return &g_storage_profile;
}

const bharat_network_profile_t *bharat_network_active_profile(void) {
    return &g_network_profile;
}

int bharat_storage_has(uint32_t feature) {
    return (g_storage_profile.features & feature) != 0U;
}

int bharat_network_has(uint32_t feature) {
    return (g_network_profile.features & feature) != 0U;
}
