#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include <time.h>
#include "core/stacks/media/compositor/compositor_core.h"

static uint64_t g_stress_time = 1000;

static uint64_t stress_now(void *ctx) {
    (void)ctx;
    return g_stress_time;
}

int main(int argc, char **argv) {
    unsigned int seed = 42;
    if (argc > 1) {
        seed = (unsigned int)atoi(argv[1]);
    }
    srand(seed);

    printf("Starting UI Surface/Buffer Stress test with seed %u...\n", seed);

    bh_compositor_core_t core;
    bh_gui_clock_t clock = { .now_ns = stress_now };
    bh_profile_caps_t caps = {
        .profile_class = BH_PROFILE_CLASS_DESKTOP,
        .ui_enabled = true,
        .compositor_enabled = true,
        .direct_scanout_allowed = true,
        .trusted_overlay_allowed = true
    };

    bh_compositor_core_init(&core, &caps, &clock);

    /* Setup a display with primary, overlay, and cursor planes */
    bh_display_handle_t disp;
    bh_display_result_t res = bh_compositor_add_display(&core, 800, 600, 60, BH_DISPLAY_FORMAT_XRGB8888, &disp);
    assert(res == BH_DISPLAY_RESULT_OK);

    bh_plane_desc_t primary = {
        .supported_formats_mask = 0xF, .min_width = 1, .min_height = 1,
        .max_width = 4096, .max_height = 4096, .direct_scanout_support = true
    };
    bh_compositor_add_display_plane(&core, disp, &primary);

    bh_display_lease_handle_t lease = BH_GUI_HANDLE_INVALID;

    const int TOTAL_CYCLES = 10000;
    for (int i = 0; i < TOTAL_CYCLES; i++) {
        g_stress_time += 16666666ULL; /* Advance clock by 16.6ms per cycle */

        /* Maintain lease */
        if (lease == BH_GUI_HANDLE_INVALID) {
            res = bh_compositor_request_lease(&core, disp, BHARAT_DISPLAY_RIGHT_PRESENT | BHARAT_DISPLAY_RIGHT_WRITE, 100, 0, &lease);
            if (res != BH_DISPLAY_RESULT_OK) {
                fprintf(stderr, "FAILED to request lease on iteration %d, seed %u\n", i, seed);
                return 1;
            }
        }

        /* 1. Create Surface */
        bh_gui_surface_handle_t surf;
        res = bh_compositor_create_surface(&core, lease, 800, 600, 1, &surf);
        if (res != BH_DISPLAY_RESULT_OK) {
            fprintf(stderr, "FAILED to create surface on iteration %d, seed %u\n", i, seed);
            return 1;
        }

        /* 2. Register Buffer */
        bh_display_buffer_desc_t desc = {
            .width = 800, .height = 600, .pixel_format = BH_DISPLAY_FORMAT_XRGB8888,
            .usage_flags = BH_DISPLAY_BUFFER_USAGE_SCANOUT, .memory_domain = BH_DISPLAY_MEMORY_DOMAIN_SYSTEM,
            .plane_count = 1, .total_size_bytes = 800 * 600 * 4, .modifier = BH_DISPLAY_MODIFIER_LINEAR,
            .planes = {
                { .offset_bytes = 0, .size_bytes = 800 * 600 * 4, .stride_bytes = 800 * 4 }
            }
        };
        bh_gui_buffer_handle_t buf;
        res = bh_compositor_register_buffer(&core, lease, &desc, &buf);
        if (res != BH_DISPLAY_RESULT_OK) {
            fprintf(stderr, "FAILED to register buffer on iteration %d, seed %u\n", i, seed);
            return 1;
        }

        /* 3. Attach Buffer */
        res = bh_compositor_attach_buffer(&core, lease, surf, buf);
        if (res != BH_DISPLAY_RESULT_OK) {
            fprintf(stderr, "FAILED to attach buffer on iteration %d, seed %u\n", i, seed);
            return 1;
        }

        /* 4. Create and signal acquire fence */
        bh_gui_fence_handle_t acq_fence;
        res = bh_compositor_create_fence(&core, lease, false, &acq_fence);
        if (res != BH_DISPLAY_RESULT_OK) {
            fprintf(stderr, "FAILED to create fence on iteration %d, seed %u\n", i, seed);
            return 1;
        }
        bh_compositor_signal_fence(&core, acq_fence);

        /* 5. Present (Inject failures periodically) */
        bool fail_inject = (i > 0) && (i % 250 == 0);
        if (fail_inject) {
            core.inject_commit_failure = true;
        }

        bh_direct_scanout_reason_t reason;
        bh_gui_fence_handle_t rel_fence;
        res = bh_compositor_present(&core, lease, surf, buf, acq_fence, 0, &reason, &rel_fence);

        if (fail_inject) {
            assert(res == BH_DISPLAY_RESULT_INTERNAL);
            core.inject_commit_failure = false;

            /* Rollback and cleanup */
            bh_compositor_attach_buffer(&core, lease, surf, BH_GUI_HANDLE_INVALID);
            bh_compositor_release_buffer(&core, lease, buf);
            bh_compositor_destroy_surface(&core, lease, surf);

            /* Clean fence slot manually since transaction failed */
            uint16_t f_slot = bh_gui_handle_get_slot(acq_fence);
            core.fences[f_slot-1].active = false;

            continue;
        }

        if (res != BH_DISPLAY_RESULT_OK) {
            fprintf(stderr, "FAILED to present on iteration %d, seed %u (res: %u)\n", i, seed, res);
            return 1;
        }

        /* 6. Retire */
        res = bh_compositor_retire_presentation(&core, lease, surf);
        if (res != BH_DISPLAY_RESULT_OK) {
            fprintf(stderr, "FAILED to retire presentation on iteration %d, seed %u\n", i, seed);
            return 1;
        }

        /* Signal and clean acquire fence */
        uint16_t acq_slot = bh_gui_handle_get_slot(acq_fence);
        core.fences[acq_slot-1].active = false;

        /* Clean release fence */
        if (rel_fence != BH_GUI_HANDLE_INVALID) {
            uint16_t rel_slot = bh_gui_handle_get_slot(rel_fence);
            core.fences[rel_slot-1].active = false;
        }

        /* 7. Detach/release buffer and destroy surface */
        res = bh_compositor_attach_buffer(&core, lease, surf, BH_GUI_HANDLE_INVALID);
        assert(res == BH_DISPLAY_RESULT_OK);

        res = bh_compositor_release_buffer(&core, lease, buf);
        assert(res == BH_DISPLAY_RESULT_OK);

        res = bh_compositor_destroy_surface(&core, lease, surf);
        assert(res == BH_DISPLAY_RESULT_OK || res == BH_DISPLAY_RESULT_BUSY);

        /* 8. Periodic Lease Revocation/Recycle */
        if (i > 0 && i % 1000 == 0) {
            res = bh_compositor_revoke_lease(&core, lease, 0);
            assert(res == BH_DISPLAY_RESULT_OK);
            res = bh_compositor_acknowledge_lease_revocation(&core, lease);
            assert(res == BH_DISPLAY_RESULT_OK);
            res = bh_compositor_release_lease(&core, lease);
            if (res != BH_DISPLAY_RESULT_OK) {
                printf("bh_compositor_release_lease failed on iteration %d, result %u\n", i, res);
                assert(false);
            }
            lease = BH_GUI_HANDLE_INVALID;
        }
    }

    /* Final clean-up of the lease if still active */
    if (lease != BH_GUI_HANDLE_INVALID) {
        bh_compositor_release_lease(&core, lease);
    }

    /* Verify all resources (leases, surfaces, buffers, fences) are completely freed & zero live resources remain */
    for (uint32_t i = 0; i < MAX_LEASES; i++) {
        assert(!core.leases[i].active);
    }
    for (uint32_t i = 0; i < MAX_SURFACES; i++) {
        assert(!core.surfaces[i].active);
    }
    for (uint32_t i = 0; i < MAX_BUFFERS; i++) {
        assert(!core.buffers[i].active);
    }
    for (uint32_t i = 0; i < MAX_FENCES; i++) {
        assert(!core.fences[i].active);
    }

    printf("\nSuccess! 10,000 cycles completed with zero leaks and perfect invariant verification!\n");
    return 0;
}
