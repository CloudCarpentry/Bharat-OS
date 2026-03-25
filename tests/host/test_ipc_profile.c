#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "profile/profile.h"
#include "ipc_endpoint.h"

static void test_profile_helpers(void) {
    assert(kernel_execution_profile_is_valid(PROFILE_KERNEL_RT) == true);
    assert(kernel_execution_profile_is_valid(PROFILE_KERNEL_GP) == true);
    assert(kernel_execution_profile_is_valid(PROFILE_KERNEL_MIX) == true);
    assert(kernel_execution_profile_is_valid((KernelExecutionProfile)999) == false);

    assert(strcmp(kernel_execution_profile_name(PROFILE_KERNEL_RT), "RT") == 0);
    assert(strcmp(kernel_execution_profile_name(PROFILE_KERNEL_GP), "GP") == 0);
    assert(strcmp(kernel_execution_profile_name(PROFILE_KERNEL_MIX), "MIX") == 0);
    assert(strcmp(kernel_execution_profile_name((KernelExecutionProfile)999), "UNKNOWN") == 0);

    // We expect it to not crash. It will return "GP" because we build with BHARAT_KERNEL_PROFILE_GP=1 during tests
    assert(strcmp(current_kernel_execution_profile_name(), "GP") == 0);
    printf("test_profile_helpers passed\n");
}

static void test_ipc_traffic_helpers(void) {
    assert(ipc_traffic_default() == IPC_TRAFFIC_UNSPECIFIED);

    assert(strcmp(ipc_traffic_type_name(IPC_TRAFFIC_UNSPECIFIED), "UNSPECIFIED") == 0);
    assert(strcmp(ipc_traffic_type_name(IPC_TRAFFIC_CONTROL), "CONTROL") == 0);
    assert(strcmp(ipc_traffic_type_name(IPC_TRAFFIC_SERVICE), "SERVICE") == 0);
    assert(strcmp(ipc_traffic_type_name(IPC_TRAFFIC_EVENT), "EVENT") == 0);
    assert(strcmp(ipc_traffic_type_name(IPC_TRAFFIC_TELEMETRY), "TELEMETRY") == 0);
    assert(strcmp(ipc_traffic_type_name(IPC_TRAFFIC_BULK), "BULK") == 0);
    assert(strcmp(ipc_traffic_type_name(IPC_TRAFFIC_DEFERRED), "DEFERRED") == 0);
    assert(strcmp(ipc_traffic_type_name((ipc_traffic_type_t)999), "UNKNOWN") == 0);
    printf("test_ipc_traffic_helpers passed\n");
}

int main(void) {
    test_profile_helpers();
    test_ipc_traffic_helpers();
    printf("All test_ipc_profile tests passed.\n");
    return 0;
}
