#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#include <core/lib/runtime/accel/tensor_backend.h>
#include <bharat/accel/accel.h>
#include <kernel/status.h>

// Extern diagnostic helpers from virt_accel driver
extern bharat_accel_device_t* get_virt_accel_mock_device(void);
extern uint64_t virt_accel_get_submit_count(void);
extern void virt_accel_reset_submit_count(void);
extern void virt_accel_set_fail_injection(bool enable);

// Floating point comparison tolerance
#define EPSILON 0.0001f
static bool float_eq(float a, float b) {
    float diff = a - b;
    if (diff < 0) diff = -diff;
    return diff < EPSILON;
}

// Global flag to track if all tests pass
static bool g_all_cases_pass = true;

// Helper to run a test and print status
#define RUN_TEST(name) \
    do { \
        printf("Running test: %s... ", #name); \
        name(); \
        printf("PASSED\n"); \
    } while (0)

/* ── Test 1: Duplicate Registration Semantics ── */
static void test_duplicate_registration(void) {
    // Re-initializing tensor dispatch twice should be handled gracefully or rejected
    // Our policy: duplicate provider identity/name -> reject with -2

    // Create a mock provider with the same name
    backend_provider_t duplicate_prov = {
        .name = "tensor_hw_generic", // Duplicate name
        .feature_class = CLASS_TENSOR_ML,
        .type = BACKEND_TYPE_GENERIC_HARDWARE,
        .priority = 99
    };

    int status = backend_registry_add(&duplicate_prov);
    assert(status == -2); // Must reject as duplicate
}

/* ── Test 2: Virtual NPU ReLU Mathematical Correctness ── */
static void test_relu_math_correctness(void) {
    // We want to verify basic mathematical execution in different bounds.
    // 1. Standard mixed array
    {
        float input[] = {-4.0f, -1.0f, 0.0f, 3.0f, 8.0f};
        float output[5] = {99.0f, 99.0f, 99.0f, 99.0f, 99.0f};

        virt_accel_job_t job = {
            .opcode = VIRT_ACCEL_OP_RELU_F32,
            .input = input,
            .input_elements = 5,
            .output = output,
            .output_elements = 5
        };

        bharat_accel_device_t *dev = get_virt_accel_mock_device();
        int ret = dev->ops->submit_npu_job(dev, &job);
        assert(ret == K_OK);

        assert(float_eq(output[0], 0.0f));
        assert(float_eq(output[1], 0.0f));
        assert(float_eq(output[2], 0.0f));
        assert(float_eq(output[3], 3.0f));
        assert(float_eq(output[4], 8.0f));
    }

    // 2. All negative
    {
        float input[] = {-10.0f, -0.1f};
        float output[2] = {99.0f, 99.0f};

        virt_accel_job_t job = {
            .opcode = VIRT_ACCEL_OP_RELU_F32,
            .input = input,
            .input_elements = 2,
            .output = output,
            .output_elements = 2
        };

        bharat_accel_device_t *dev = get_virt_accel_mock_device();
        int ret = dev->ops->submit_npu_job(dev, &job);
        assert(ret == K_OK);
        assert(float_eq(output[0], 0.0f));
        assert(float_eq(output[1], 0.0f));
    }

    // 3. All positive
    {
        float input[] = {5.5f, 100.0f};
        float output[2] = {99.0f, 99.0f};

        virt_accel_job_t job = {
            .opcode = VIRT_ACCEL_OP_RELU_F32,
            .input = input,
            .input_elements = 2,
            .output = output,
            .output_elements = 2
        };

        bharat_accel_device_t *dev = get_virt_accel_mock_device();
        int ret = dev->ops->submit_npu_job(dev, &job);
        assert(ret == K_OK);
        assert(float_eq(output[0], 5.5f));
        assert(float_eq(output[1], 100.0f));
    }

    // 4. Zero & Single element
    {
        float input[] = {0.0f};
        float output[1] = {99.0f};

        virt_accel_job_t job = {
            .opcode = VIRT_ACCEL_OP_RELU_F32,
            .input = input,
            .input_elements = 1,
            .output = output,
            .output_elements = 1
        };

        bharat_accel_device_t *dev = get_virt_accel_mock_device();
        int ret = dev->ops->submit_npu_job(dev, &job);
        assert(ret == K_OK);
        assert(float_eq(output[0], 0.0f));
    }
}

/* ── Test 3: Invalid Input/Descriptor Validation ── */
static void test_invalid_descriptors(void) {
    bharat_accel_device_t *dev = get_virt_accel_mock_device();

    // 1. NULL job descriptor
    int ret = dev->ops->submit_npu_job(dev, NULL);
    assert(ret == K_ERR_INVALID_ARG);

    // 2. NULL input
    {
        float output[2];
        virt_accel_job_t job = {
            .opcode = VIRT_ACCEL_OP_RELU_F32,
            .input = NULL,
            .input_elements = 2,
            .output = output,
            .output_elements = 2
        };
        assert(dev->ops->submit_npu_job(dev, &job) == K_ERR_INVALID_ARG);
    }

    // 3. NULL output
    {
        float input[2] = {1.0f, 2.0f};
        virt_accel_job_t job = {
            .opcode = VIRT_ACCEL_OP_RELU_F32,
            .input = input,
            .input_elements = 2,
            .output = NULL,
            .output_elements = 2
        };
        assert(dev->ops->submit_npu_job(dev, &job) == K_ERR_INVALID_ARG);
    }

    // 4. Zero elements
    {
        float input[2] = {1.0f, 2.0f};
        float output[2];
        virt_accel_job_t job = {
            .opcode = VIRT_ACCEL_OP_RELU_F32,
            .input = input,
            .input_elements = 0,
            .output = output,
            .output_elements = 2
        };
        assert(dev->ops->submit_npu_job(dev, &job) == K_ERR_INVALID_ARG);
    }

    // 5. Output too small (overflow)
    {
        float input[5] = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f};
        float output[2];
        virt_accel_job_t job = {
            .opcode = VIRT_ACCEL_OP_RELU_F32,
            .input = input,
            .input_elements = 5,
            .output = output,
            .output_elements = 2
        };
        assert(dev->ops->submit_npu_job(dev, &job) == K_ERR_OVERFLOW);
    }

    // 6. Unsupported Opcode
    {
        float input[2] = {1.0f, 2.0f};
        float output[2];
        virt_accel_job_t job = {
            .opcode = (virt_accel_opcode_t)9999, // Unsupported
            .input = input,
            .input_elements = 2,
            .output = output,
            .output_elements = 2
        };
        assert(dev->ops->submit_npu_job(dev, &job) == K_ERR_UNSUPPORTED);
    }
}

/* ── Test 4: Scenario A — Normal path (NPU available, safe_mode false) ── */
static void test_scenario_normal(void) {
    tensor_dispatch_init();
    virt_accel_reset_submit_count();
    virt_accel_set_fail_injection(false);

    bharat_hw_caps_t caps;
    memset(&caps, 0, sizeof(caps));
    caps.soc.npu = HW_CAP_STATE_PRESENT; // NPU available

    backend_dispatch_context_t ctx = {
        .power_level = 100,
        .safe_mode = false, // normal mode
        .qos_level = 1
    };

    float input[] = {-4.0f, -1.0f, 0.0f, 3.0f, 8.0f};
    float output[5] = {99.0f, 99.0f, 99.0f, 99.0f, 99.0f};

    tensor_op_t op = {
        .op = TENSOR_OP_RELU,
        .input_tensor = input,
        .in_elements = 5,
        .output_tensor = output,
        .out_elements = 5
    };

    uint64_t before_count = virt_accel_get_submit_count();

    // Process
    int ret = tensor_process(&op, &caps, &ctx);
    assert(ret == K_OK);

    // Verify emulated driver execution actually occurred
    uint64_t after_count = virt_accel_get_submit_count();
    assert(after_count == before_count + 1);

    // Verify mathematical correctness
    assert(float_eq(output[0], 0.0f));
    assert(float_eq(output[1], 0.0f));
    assert(float_eq(output[2], 0.0f));
    assert(float_eq(output[3], 3.0f));
    assert(float_eq(output[4], 8.0f));

    // Verify stats
    tensor_dispatch_stats_t stats;
    tensor_dispatch_get_stats(&stats);
    assert(stats.backend_hw_selected == 1);
    assert(stats.backend_sw_fallback == 0);
    assert(stats.jobs_completed == 1);
    assert(stats.jobs_failed == 0);

    // Verify last result
    tensor_dispatch_result_t result;
    tensor_dispatch_get_last_result(&result);
    assert(strcmp(result.backend_name, "tensor_hw_generic") == 0);
    assert(result.backend_type == BACKEND_TYPE_GENERIC_HARDWARE);
    assert(result.execution_status == K_OK);

    // Emit demonstration markers
    printf("\nBHARAT_HETERO_DEMO:CASE=NORMAL\n");
    printf("BHARAT_HETERO_DEMO:BACKEND=%s\n", result.backend_name);
    printf("BHARAT_HETERO_DEMO:RESULT=PASS\n");
}

/* ── Test 5: Scenario B — Safe-Mode path (NPU available, safe_mode true) ── */
static void test_scenario_safe_mode(void) {
    tensor_dispatch_init();
    virt_accel_reset_submit_count();
    virt_accel_set_fail_injection(false);

    bharat_hw_caps_t caps;
    memset(&caps, 0, sizeof(caps));
    caps.soc.npu = HW_CAP_STATE_PRESENT; // NPU available

    backend_dispatch_context_t ctx = {
        .power_level = 100,
        .safe_mode = true, // policy rejects hardware!
        .qos_level = 1
    };

    float input[] = {-4.0f, -1.0f, 0.0f, 3.0f, 8.0f};
    float output[5] = {99.0f, 99.0f, 99.0f, 99.0f, 99.0f};

    tensor_op_t op = {
        .op = TENSOR_OP_RELU,
        .input_tensor = input,
        .in_elements = 5,
        .output_tensor = output,
        .out_elements = 5
    };

    uint64_t before_count = virt_accel_get_submit_count();

    // Process
    int ret = tensor_process(&op, &caps, &ctx);
    assert(ret == K_OK);

    // Verify emulated driver execution did NOT occur (submit count unchanged)
    uint64_t after_count = virt_accel_get_submit_count();
    assert(after_count == before_count);

    // Verify software output correctness
    assert(float_eq(output[0], 0.0f));
    assert(float_eq(output[1], 0.0f));
    assert(float_eq(output[2], 0.0f));
    assert(float_eq(output[3], 3.0f));
    assert(float_eq(output[4], 8.0f));

    // Verify stats
    tensor_dispatch_stats_t stats;
    tensor_dispatch_get_stats(&stats);
    assert(stats.backend_hw_selected == 0);
    assert(stats.backend_sw_fallback == 1);
    assert(stats.safe_mode_fallbacks == 1);
    assert(stats.unavailable_fallbacks == 0);
    assert(stats.jobs_completed == 1);
    assert(stats.jobs_failed == 0);

    // Verify last result
    tensor_dispatch_result_t result;
    tensor_dispatch_get_last_result(&result);
    assert(strcmp(result.backend_name, "tensor_sw_fallback") == 0);
    assert(result.backend_type == BACKEND_TYPE_SOFTWARE_FALLBACK);
    assert(result.execution_status == K_OK);

    // Emit demonstration markers
    printf("\nBHARAT_HETERO_DEMO:CASE=SAFE_MODE\n");
    printf("BHARAT_HETERO_DEMO:BACKEND=CPU\n"); // General display string as required
    printf("BHARAT_HETERO_DEMO:RESULT=PASS\n");
}

/* ── Test 6: Scenario C — NPU Absent Fallback ── */
static void test_scenario_npu_absent(void) {
    tensor_dispatch_init();
    virt_accel_reset_submit_count();
    virt_accel_set_fail_injection(false);

    bharat_hw_caps_t caps;
    memset(&caps, 0, sizeof(caps));
    caps.soc.npu = HW_CAP_STATE_ABSENT; // NPU absent/unavailable

    backend_dispatch_context_t ctx = {
        .power_level = 100,
        .safe_mode = false,
        .qos_level = 1
    };

    float input[] = {-4.0f, -1.0f, 0.0f, 3.0f, 8.0f};
    float output[5] = {99.0f, 99.0f, 99.0f, 99.0f, 99.0f};

    tensor_op_t op = {
        .op = TENSOR_OP_RELU,
        .input_tensor = input,
        .in_elements = 5,
        .output_tensor = output,
        .out_elements = 5
    };

    uint64_t before_count = virt_accel_get_submit_count();

    // Process
    int ret = tensor_process(&op, &caps, &ctx);
    assert(ret == K_OK);

    // Verify emulated driver execution did NOT occur (submit count unchanged)
    uint64_t after_count = virt_accel_get_submit_count();
    assert(after_count == before_count);

    // Verify software output correctness
    assert(float_eq(output[0], 0.0f));
    assert(float_eq(output[1], 0.0f));
    assert(float_eq(output[2], 0.0f));
    assert(float_eq(output[3], 3.0f));
    assert(float_eq(output[4], 8.0f));

    // Verify stats
    tensor_dispatch_stats_t stats;
    tensor_dispatch_get_stats(&stats);
    assert(stats.backend_hw_selected == 0);
    assert(stats.backend_sw_fallback == 1);
    assert(stats.safe_mode_fallbacks == 0);
    assert(stats.unavailable_fallbacks == 1);
    assert(stats.jobs_completed == 1);
    assert(stats.jobs_failed == 0);

    // Verify last result
    tensor_dispatch_result_t result;
    tensor_dispatch_get_last_result(&result);
    assert(strcmp(result.backend_name, "tensor_sw_fallback") == 0);
    assert(result.backend_type == BACKEND_TYPE_SOFTWARE_FALLBACK);
    assert(result.execution_status == K_OK);

    // Emit demonstration markers
    printf("\nBHARAT_HETERO_DEMO:CASE=NPU_ABSENT\n");
    printf("BHARAT_HETERO_DEMO:BACKEND=CPU\n");
    printf("BHARAT_HETERO_DEMO:RESULT=PASS\n");
}

/* ── Test 7: Hardware Selected then Execution Fails (Failure Propagation) ── */
static void test_hardware_execution_failure(void) {
    tensor_dispatch_init();
    virt_accel_reset_submit_count();

    // Enable failure injection inside the virtual accelerator driver
    virt_accel_set_fail_injection(true);

    bharat_hw_caps_t caps;
    memset(&caps, 0, sizeof(caps));
    caps.soc.npu = HW_CAP_STATE_PRESENT; // NPU available, so routing will select NPU

    backend_dispatch_context_t ctx = {
        .power_level = 100,
        .safe_mode = false,
        .qos_level = 1
    };

    float input[] = {-4.0f, -1.0f, 0.0f, 3.0f, 8.0f};
    float output[5] = {99.0f, 99.0f, 99.0f, 99.0f, 99.0f};

    tensor_op_t op = {
        .op = TENSOR_OP_RELU,
        .input_tensor = input,
        .in_elements = 5,
        .output_tensor = output,
        .out_elements = 5
    };

    uint64_t before_count = virt_accel_get_submit_count();

    // Process: must return failure since virtual NPU failed!
    int ret = tensor_process(&op, &caps, &ctx);
    assert(ret == K_ERR_DEV_MMIO_FAULT);

    // Check that invocation actually went to hardware driver and incremented submit count
    uint64_t after_count = virt_accel_get_submit_count();
    assert(after_count == before_count + 1);

    // Verify stats: must NOT silently fallback, but instead report failure
    tensor_dispatch_stats_t stats;
    tensor_dispatch_get_stats(&stats);
    assert(stats.backend_hw_selected == 1);
    assert(stats.backend_sw_fallback == 0);
    assert(stats.jobs_completed == 0);
    assert(stats.jobs_failed == 1); // Failure is truthfully recorded!

    // Clean up failure injection
    virt_accel_set_fail_injection(false);
}

/* ── MAIN ── */
int main(void) {
    printf("BHARAT_HETERO_DEMO:START\n");
    printf("BHARAT_HETERO_DEMO:VIRT_NPU=AVAILABLE\n\n");

    // Initialize once
    int init_status = tensor_dispatch_init();
    assert(init_status == 0);

    // Run tests
    RUN_TEST(test_relu_math_correctness);
    RUN_TEST(test_invalid_descriptors);
    RUN_TEST(test_scenario_normal);
    RUN_TEST(test_scenario_safe_mode);
    RUN_TEST(test_scenario_npu_absent);
    RUN_TEST(test_hardware_execution_failure);
    RUN_TEST(test_duplicate_registration);

    printf("\nAll heterogeneous compute dispatch tests passed successfully!\n");

    if (g_all_cases_pass) {
        printf("BHARAT_HETERO_DEMO:COMPLETE\n");
    }

    return 0;
}
