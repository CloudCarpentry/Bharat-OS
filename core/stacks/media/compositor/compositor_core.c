#include "compositor_core.h"
#ifndef BHARAT_HOST_TEST
#include <stddef.h>
void *memset(void *s, int c, size_t n);
#else
#include <string.h>
#endif

/* Overflow-safe helpers */
static inline bool check_mul_overflow_u32(uint32_t a, uint32_t b, uint32_t *res) {
    uint64_t val = (uint64_t)a * b;
    if (val > UINT32_MAX) return true;
    *res = (uint32_t)val;
    return false;
}

static inline bool check_add_overflow_u64(uint64_t a, uint64_t b, uint64_t *res) {
    if (UINT64_MAX - a < b) return true;
    *res = a + b;
    return false;
}

/* Initialization */
void bh_compositor_core_init(bh_compositor_core_t *core, const bh_profile_caps_t *caps, const bh_gui_clock_t *clock) {
    if (!core) return;
    memset(core, 0, sizeof(*core));
    if (caps) core->profile_caps = *caps;
    if (clock) core->clock = *clock;

    /* Initialize generations of all entries to 1 */
    for (uint32_t i = 0; i < MAX_DISPLAYS; i++) {
        core->displays[i].generation = 1;
        core->displays[i].active_lease = BH_GUI_HANDLE_INVALID;
    }
    for (uint32_t i = 0; i < MAX_LEASES; i++) {
        core->leases[i].generation = 1;
    }
    for (uint32_t i = 0; i < MAX_SURFACES; i++) {
        core->surfaces[i].generation = 1;
    }
    for (uint32_t i = 0; i < MAX_BUFFERS; i++) {
        core->buffers[i].generation = 1;
    }
    for (uint32_t i = 0; i < MAX_FENCES; i++) {
        core->fences[i].generation = 1;
    }
}

/* Add Display */
bh_display_result_t bh_compositor_add_display(bh_compositor_core_t *core, uint32_t w, uint32_t h, uint32_t refresh, uint32_t format, bh_display_handle_t *out_handle) {
    if (!core->profile_caps.ui_enabled) return BH_DISPLAY_RESULT_PROFILE_UNSUPPORTED;

    uint32_t found_idx = UINT32_MAX;
    for (uint32_t i = 0; i < MAX_DISPLAYS; i++) {
        if (!core->displays[i].active) {
            found_idx = i;
            break;
        }
    }
    if (found_idx == UINT32_MAX) return BH_DISPLAY_RESULT_NO_RESOURCES;

    bh_display_entry_t *disp = &core->displays[found_idx];
    disp->active = true;
    disp->width = w;
    disp->height = h;
    disp->refresh_hz = refresh;
    disp->pixel_format = format;
    disp->flags = 0;
    disp->active_lease = BH_GUI_HANDLE_INVALID;
    disp->plane_count = 0;

    disp->handle = bh_gui_handle_pack(found_idx + 1, disp->generation, BH_GUI_RESOURCE_DISPLAY, BH_GUI_HANDLE_ABI_V1);
    if (out_handle) *out_handle = disp->handle;
    return BH_DISPLAY_RESULT_OK;
}

bh_display_result_t bh_compositor_add_display_plane(bh_compositor_core_t *core, bh_display_handle_t disp_handle, const bh_plane_desc_t *plane) {
    if (!bh_gui_handle_validate(disp_handle, BH_GUI_RESOURCE_DISPLAY)) return BH_DISPLAY_RESULT_INVALID_ARGUMENT;
    uint16_t slot = bh_gui_handle_get_slot(disp_handle);
    uint32_t idx = slot - 1;
    if (idx >= MAX_DISPLAYS || !core->displays[idx].active || core->displays[idx].handle != disp_handle) {
        return BH_DISPLAY_RESULT_NOT_FOUND;
    }

    bh_display_entry_t *disp = &core->displays[idx];
    if (disp->plane_count >= BH_DISPLAY_MAX_PLANES) return BH_DISPLAY_RESULT_NO_RESOURCES;

    disp->planes[disp->plane_count] = *plane;
    disp->planes[disp->plane_count].plane_id = disp->plane_count;
    disp->plane_count++;
    return BH_DISPLAY_RESULT_OK;
}

/* Lease Operations */
bh_display_result_t bh_compositor_request_lease(bh_compositor_core_t *core, bh_display_handle_t disp_handle, uint32_t requested_rights, uint32_t client_pid, uint64_t endpoint, bh_display_lease_handle_t *out_lease) {
    if (!core->profile_caps.ui_enabled) return BH_DISPLAY_RESULT_PROFILE_UNSUPPORTED;

    if (!bh_gui_handle_validate(disp_handle, BH_GUI_RESOURCE_DISPLAY)) return BH_DISPLAY_RESULT_INVALID_ARGUMENT;
    uint16_t disp_slot = bh_gui_handle_get_slot(disp_handle);
    uint32_t disp_idx = disp_slot - 1;
    if (disp_idx >= MAX_DISPLAYS || !core->displays[disp_idx].active || core->displays[disp_idx].handle != disp_handle) {
        return BH_DISPLAY_RESULT_NOT_FOUND;
    }

    bh_display_entry_t *disp = &core->displays[disp_idx];

    /* Policy check for rights */
    if (requested_rights == 0) return BH_DISPLAY_RESULT_INVALID_ARGUMENT;

    /* If there is already an active lease, we initiate revocation on it */
    if (disp->active_lease != BH_GUI_HANDLE_INVALID) {
        bh_compositor_revoke_lease(core, disp->active_lease, 1000000000ULL); /* 1s default */
    }

    uint32_t found_idx = UINT32_MAX;
    for (uint32_t i = 0; i < MAX_LEASES; i++) {
        if (!core->leases[i].active) {
            found_idx = i;
            break;
        }
    }
    if (found_idx == UINT32_MAX) return BH_DISPLAY_RESULT_NO_RESOURCES;

    bh_lease_entry_t *lease = &core->leases[found_idx];
    lease->active = true;
    lease->display_handle = disp_handle;
    lease->rights = requested_rights;
    lease->state = BHARAT_DISPLAY_LEASE_STATE_ACTIVE;
    lease->client_pid = client_pid;
    lease->client_endpoint = endpoint;
    lease->revocation_deadline = 0;

    lease->handle = bh_gui_handle_pack(found_idx + 1, lease->generation, BH_GUI_RESOURCE_LEASE, BH_GUI_HANDLE_ABI_V1);
    disp->active_lease = lease->handle;

    if (out_lease) *out_lease = lease->handle;
    return BH_DISPLAY_RESULT_OK;
}

bh_display_result_t bh_compositor_revoke_lease(bh_compositor_core_t *core, bh_display_lease_handle_t lease_handle, uint64_t grace_period_ns) {
    if (!bh_gui_handle_validate(lease_handle, BH_GUI_RESOURCE_LEASE)) return BH_DISPLAY_RESULT_INVALID_ARGUMENT;
    uint16_t slot = bh_gui_handle_get_slot(lease_handle);
    uint32_t idx = slot - 1;
    if (idx >= MAX_LEASES || !core->leases[idx].active || core->leases[idx].handle != lease_handle) {
        return BH_DISPLAY_RESULT_NOT_FOUND;
    }

    bh_lease_entry_t *lease = &core->leases[idx];
    if (lease->state == BHARAT_DISPLAY_LEASE_STATE_REVOKED) return BH_DISPLAY_RESULT_OK;

    lease->state = BHARAT_DISPLAY_LEASE_STATE_REVOKING;
    if (core->clock.now_ns) {
        lease->revocation_deadline = core->clock.now_ns(core->clock.ctx) + grace_period_ns;
    } else {
        lease->revocation_deadline = grace_period_ns;
    }

    /* Prevent future queueing for all buffers tied to this lease */
    for (uint32_t i = 0; i < MAX_BUFFERS; i++) {
        if (core->buffers[i].active && core->buffers[i].lease_handle == lease_handle) {
            if (core->buffers[i].state != BH_BUFFER_STATE_SCANNING_OUT && core->buffers[i].state != BH_BUFFER_STATE_QUEUED) {
                core->buffers[i].state = BH_BUFFER_STATE_REVOKED;
            }
        }
    }

    return BH_DISPLAY_RESULT_OK;
}

bh_display_result_t bh_compositor_acknowledge_lease_revocation(bh_compositor_core_t *core, bh_display_lease_handle_t lease_handle) {
    if (!bh_gui_handle_validate(lease_handle, BH_GUI_RESOURCE_LEASE)) return BH_DISPLAY_RESULT_INVALID_ARGUMENT;
    uint16_t slot = bh_gui_handle_get_slot(lease_handle);
    uint32_t idx = slot - 1;
    if (idx >= MAX_LEASES || !core->leases[idx].active || core->leases[idx].handle != lease_handle) {
        return BH_DISPLAY_RESULT_NOT_FOUND;
    }

    bh_lease_entry_t *lease = &core->leases[idx];
    if (lease->state != BHARAT_DISPLAY_LEASE_STATE_REVOKING) {
        return BH_DISPLAY_RESULT_BAD_STATE;
    }

    lease->state = BHARAT_DISPLAY_LEASE_STATE_REVOKED;
    return BH_DISPLAY_RESULT_OK;
}

bh_display_result_t bh_compositor_release_lease(bh_compositor_core_t *core, bh_display_lease_handle_t lease_handle) {
    if (!bh_gui_handle_validate(lease_handle, BH_GUI_RESOURCE_LEASE)) return BH_DISPLAY_RESULT_INVALID_ARGUMENT;
    uint16_t slot = bh_gui_handle_get_slot(lease_handle);
    uint32_t idx = slot - 1;
    if (idx >= MAX_LEASES || !core->leases[idx].active || core->leases[idx].handle != lease_handle) {
        return BH_DISPLAY_RESULT_NOT_FOUND;
    }

    bh_lease_entry_t *lease = &core->leases[idx];

    /* Force stop scanout and release all resources associated with this lease */
    for (uint32_t i = 0; i < MAX_SURFACES; i++) {
        if (core->surfaces[i].active && core->surfaces[i].lease_handle == lease_handle) {
            core->surfaces[i].scanning_buffer = BH_GUI_HANDLE_INVALID;
            core->surfaces[i].retired_buffer = BH_GUI_HANDLE_INVALID;
            core->surfaces[i].attached_buffer = BH_GUI_HANDLE_INVALID;
            core->surfaces[i].state = BH_SURFACE_STATE_DESTROYED;
            core->surfaces[i].active = false;
            if (core->surfaces[i].generation < UINT32_MAX) {
                core->surfaces[i].generation++;
            } else {
                core->surfaces[i].generation = 0;
            }
        }
    }

    for (uint32_t i = 0; i < MAX_BUFFERS; i++) {
        if (core->buffers[i].active && core->buffers[i].lease_handle == lease_handle) {
            core->buffers[i].state = BH_BUFFER_STATE_FREE;
            core->buffers[i].active = false;
            if (core->buffers[i].generation < UINT32_MAX) {
                core->buffers[i].generation++;
            } else {
                core->buffers[i].generation = 0;
            }
        }
    }

    /* Clear display lease association */
    uint16_t disp_slot = bh_gui_handle_get_slot(lease->display_handle);
    uint32_t disp_idx = disp_slot - 1;
    if (disp_idx < MAX_DISPLAYS && core->displays[disp_idx].active_lease == lease_handle) {
        core->displays[disp_idx].active_lease = BH_GUI_HANDLE_INVALID;
    }

    lease->active = false;
    if (lease->generation < UINT32_MAX) {
        lease->generation++;
    } else {
        lease->generation = 0;
    }
    return BH_DISPLAY_RESULT_OK;
}

/* Surface Lifecycle */
bh_display_result_t bh_compositor_create_surface(bh_compositor_core_t *core, bh_display_lease_handle_t lease_handle, uint32_t w, uint32_t h, uint32_t z_order, bh_gui_surface_handle_t *out_surface) {
    if (!core->profile_caps.ui_enabled) return BH_DISPLAY_RESULT_PROFILE_UNSUPPORTED;

    if (!bh_gui_handle_validate(lease_handle, BH_GUI_RESOURCE_LEASE)) return BH_DISPLAY_RESULT_INVALID_ARGUMENT;
    uint16_t lease_slot = bh_gui_handle_get_slot(lease_handle);
    uint32_t lease_idx = lease_slot - 1;
    if (lease_idx >= MAX_LEASES || !core->leases[lease_idx].active || core->leases[lease_idx].handle != lease_handle) {
        return BH_DISPLAY_RESULT_NOT_FOUND;
    }
    if (core->leases[lease_idx].state == BHARAT_DISPLAY_LEASE_STATE_REVOKED) {
        return BH_DISPLAY_RESULT_REVOKED;
    }

    if (w == 0 || h == 0) return BH_DISPLAY_RESULT_INVALID_ARGUMENT;

    uint32_t found_idx = UINT32_MAX;
    for (uint32_t i = 0; i < MAX_SURFACES; i++) {
        if (!core->surfaces[i].active) {
            found_idx = i;
            break;
        }
    }
    if (found_idx == UINT32_MAX) return BH_DISPLAY_RESULT_NO_RESOURCES;

    bh_surface_entry_t *surf = &core->surfaces[found_idx];
    surf->active = true;
    surf->lease_handle = lease_handle;
    surf->width = w;
    surf->height = h;
    surf->z_order = z_order;
    surf->state = BH_SURFACE_STATE_CREATED;
    surf->attached_buffer = BH_GUI_HANDLE_INVALID;
    surf->scanning_buffer = BH_GUI_HANDLE_INVALID;
    surf->retired_buffer = BH_GUI_HANDLE_INVALID;
    surf->frame_counter = 0;

    surf->handle = bh_gui_handle_pack(found_idx + 1, surf->generation, BH_GUI_RESOURCE_SURFACE, BH_GUI_HANDLE_ABI_V1);
    if (out_surface) *out_surface = surf->handle;
    return BH_DISPLAY_RESULT_OK;
}

bh_display_result_t bh_compositor_destroy_surface(bh_compositor_core_t *core, bh_display_lease_handle_t lease_handle, bh_gui_surface_handle_t surface_handle) {
    if (!bh_gui_handle_validate(surface_handle, BH_GUI_RESOURCE_SURFACE)) return BH_DISPLAY_RESULT_INVALID_ARGUMENT;
    uint16_t slot = bh_gui_handle_get_slot(surface_handle);
    uint32_t idx = slot - 1;
    if (idx >= MAX_SURFACES || !core->surfaces[idx].active || core->surfaces[idx].handle != surface_handle) {
        return BH_DISPLAY_RESULT_NOT_FOUND;
    }

    bh_surface_entry_t *surf = &core->surfaces[idx];
    if (surf->lease_handle != lease_handle) return BH_DISPLAY_RESULT_PERMISSION_DENIED;

    if (surf->state == BH_SURFACE_STATE_DESTROYED) return BH_DISPLAY_RESULT_OK;

    if (surf->scanning_buffer != BH_GUI_HANDLE_INVALID) {
        surf->state = BH_SURFACE_STATE_DESTROY_PENDING;
        return BH_DISPLAY_RESULT_BUSY;
    }

    /* Detach any attached buffer */
    if (surf->attached_buffer != BH_GUI_HANDLE_INVALID) {
        uint16_t buf_slot = bh_gui_handle_get_slot(surf->attached_buffer);
        uint32_t buf_idx = buf_slot - 1;
        if (buf_idx < MAX_BUFFERS && core->buffers[buf_idx].active && core->buffers[buf_idx].handle == surf->attached_buffer) {
            core->buffers[buf_idx].state = BH_BUFFER_STATE_ALLOCATED;
            core->buffers[buf_idx].attached_surface = BH_GUI_HANDLE_INVALID;
        }
        surf->attached_buffer = BH_GUI_HANDLE_INVALID;
    }

    surf->state = BH_SURFACE_STATE_DESTROYED;
    surf->active = false;
    if (surf->generation < UINT32_MAX) {
        surf->generation++;
    } else {
        surf->generation = 0;
    }
    return BH_DISPLAY_RESULT_OK;
}

/* Buffer Descriptor validation */
bh_display_result_t bh_compositor_validate_buffer_desc(const bh_display_buffer_desc_t *desc) {
    if (!desc) return BH_DISPLAY_RESULT_INVALID_ARGUMENT;
    if (desc->width == 0 || desc->height == 0) return BH_DISPLAY_RESULT_INVALID_ARGUMENT;
    if (desc->plane_count == 0 || desc->plane_count > BH_DISPLAY_MAX_PLANES) return BH_DISPLAY_RESULT_INVALID_ARGUMENT;

    /* Validate arithmetic overflow on dimensions */
    uint32_t dummy;
    if (check_mul_overflow_u32(desc->width, desc->height, &dummy)) return BH_DISPLAY_RESULT_DESCRIPTOR_INVALID;

    /* Validate format */
    if (desc->pixel_format != BH_DISPLAY_FORMAT_XRGB8888 &&
        desc->pixel_format != BH_DISPLAY_FORMAT_ARGB8888 &&
        desc->pixel_format != BH_DISPLAY_FORMAT_RGB565 &&
        desc->pixel_format != BH_DISPLAY_FORMAT_NV12) {
        return BH_DISPLAY_RESULT_FORMAT_UNSUPPORTED;
    }

    /* Modifier check */
    if (desc->modifier != BH_DISPLAY_MODIFIER_LINEAR) {
        return BH_DISPLAY_RESULT_MODIFIER_UNSUPPORTED;
    }

    /* Domain and usage checks */
    if (desc->memory_domain == BH_DISPLAY_MEMORY_DOMAIN_INVALID) return BH_DISPLAY_RESULT_MEMORY_DOMAIN_UNSUPPORTED;
    if (desc->usage_flags == 0) return BH_DISPLAY_RESULT_DESCRIPTOR_INVALID;

    /* Protected memory constraint */
    if ((desc->usage_flags & BH_DISPLAY_BUFFER_USAGE_PROTECTED) &&
        ((desc->usage_flags & BH_DISPLAY_BUFFER_USAGE_CPU_READ) || (desc->usage_flags & BH_DISPLAY_BUFFER_USAGE_CPU_WRITE))) {
        return BH_DISPLAY_RESULT_DESCRIPTOR_INVALID;
    }

    /* Direct scanout usage constraint */
    if ((desc->usage_flags & BH_DISPLAY_BUFFER_USAGE_SCANOUT) == 0) {
        /* Can still register it if we only compose, but must check direct scanout usage flags during evaluation */
    }

    /* Validate plane offsets and strides */
    uint64_t total_calc_size = 0;
    for (uint32_t i = 0; i < desc->plane_count; i++) {
        const bh_display_buffer_plane_t *plane = &desc->planes[i];
        if (plane->size_bytes == 0) return BH_DISPLAY_RESULT_DESCRIPTOR_INVALID;
        if (plane->stride_bytes == 0) return BH_DISPLAY_RESULT_DESCRIPTOR_INVALID;

        uint64_t end_offset;
        if (check_add_overflow_u64(plane->offset_bytes, plane->size_bytes, &end_offset)) {
            return BH_DISPLAY_RESULT_DESCRIPTOR_INVALID;
        }
        if (end_offset > desc->total_size_bytes) {
            return BH_DISPLAY_RESULT_DESCRIPTOR_INVALID;
        }

        /* Basic stride validation */
        uint32_t min_stride = desc->width;
        if (desc->pixel_format == BH_DISPLAY_FORMAT_XRGB8888 || desc->pixel_format == BH_DISPLAY_FORMAT_ARGB8888) {
            min_stride = desc->width * 4;
        } else if (desc->pixel_format == BH_DISPLAY_FORMAT_RGB565) {
            min_stride = desc->width * 2;
        }
        if (plane->stride_bytes < min_stride && desc->pixel_format != BH_DISPLAY_FORMAT_NV12) {
            return BH_DISPLAY_RESULT_DESCRIPTOR_INVALID;
        }
    }

    /* Check reserved fields */
    if (desc->abi_version != 0 && desc->abi_version != BH_GUI_HANDLE_ABI_V1) {
        return BH_DISPLAY_RESULT_DESCRIPTOR_INVALID;
    }

    return BH_DISPLAY_RESULT_OK;
}

/* Register Buffer */
bh_display_result_t bh_compositor_register_buffer(bh_compositor_core_t *core, bh_display_lease_handle_t lease_handle, const bh_display_buffer_desc_t *desc, bh_gui_buffer_handle_t *out_buffer) {
    if (!core->profile_caps.ui_enabled) return BH_DISPLAY_RESULT_PROFILE_UNSUPPORTED;

    if (!bh_gui_handle_validate(lease_handle, BH_GUI_RESOURCE_LEASE)) return BH_DISPLAY_RESULT_INVALID_ARGUMENT;
    uint16_t lease_slot = bh_gui_handle_get_slot(lease_handle);
    uint32_t lease_idx = lease_slot - 1;
    if (lease_idx >= MAX_LEASES || !core->leases[lease_idx].active || core->leases[lease_idx].handle != lease_handle) {
        return BH_DISPLAY_RESULT_NOT_FOUND;
    }
    if (core->leases[lease_idx].state == BHARAT_DISPLAY_LEASE_STATE_REVOKED) {
        return BH_DISPLAY_RESULT_REVOKED;
    }

    bh_display_result_t desc_res = bh_compositor_validate_buffer_desc(desc);
    if (desc_res != BH_DISPLAY_RESULT_OK) return desc_res;

    uint32_t found_idx = UINT32_MAX;
    for (uint32_t i = 0; i < MAX_BUFFERS; i++) {
        if (!core->buffers[i].active) {
            found_idx = i;
            break;
        }
    }
    if (found_idx == UINT32_MAX) return BH_DISPLAY_RESULT_NO_RESOURCES;

    bh_buffer_entry_t *buf = &core->buffers[found_idx];
    if (buf->generation == 0) return BH_DISPLAY_RESULT_NO_RESOURCES; /* permanently retired slot */

    buf->active = true;
    buf->lease_handle = lease_handle;
    buf->desc = *desc;
    buf->state = BH_BUFFER_STATE_ALLOCATED;
    buf->attached_surface = BH_GUI_HANDLE_INVALID;
    buf->release_fence = BH_GUI_HANDLE_INVALID;

    buf->handle = bh_gui_handle_pack(found_idx + 1, buf->generation, BH_GUI_RESOURCE_BUFFER, BH_GUI_HANDLE_ABI_V1);
    if (out_buffer) *out_buffer = buf->handle;
    return BH_DISPLAY_RESULT_OK;
}

bh_display_result_t bh_compositor_release_buffer(bh_compositor_core_t *core, bh_display_lease_handle_t lease_handle, bh_gui_buffer_handle_t buffer_handle) {
    if (!bh_gui_handle_validate(buffer_handle, BH_GUI_RESOURCE_BUFFER)) return BH_DISPLAY_RESULT_INVALID_ARGUMENT;
    uint16_t slot = bh_gui_handle_get_slot(buffer_handle);
    uint32_t idx = slot - 1;
    if (idx >= MAX_BUFFERS || !core->buffers[idx].active || core->buffers[idx].handle != buffer_handle) {
        return BH_DISPLAY_RESULT_NOT_FOUND;
    }

    bh_buffer_entry_t *buf = &core->buffers[idx];
    if (buf->lease_handle != lease_handle) return BH_DISPLAY_RESULT_PERMISSION_DENIED;

    if (buf->state == BH_BUFFER_STATE_QUEUED || buf->state == BH_BUFFER_STATE_SCANNING_OUT) {
        return BH_DISPLAY_RESULT_BUSY;
    }

    /* Detach from surface if needed */
    if (buf->attached_surface != BH_GUI_HANDLE_INVALID) {
        uint16_t surf_slot = bh_gui_handle_get_slot(buf->attached_surface);
        uint32_t surf_idx = surf_slot - 1;
        if (surf_idx < MAX_SURFACES && core->surfaces[surf_idx].active && core->surfaces[surf_idx].attached_buffer == buffer_handle) {
            core->surfaces[surf_idx].attached_buffer = BH_GUI_HANDLE_INVALID;
        }
        buf->attached_surface = BH_GUI_HANDLE_INVALID;
    }

    buf->state = BH_BUFFER_STATE_RELEASED;
    buf->active = false;
    if (buf->generation < UINT32_MAX) {
        buf->generation++;
    } else {
        buf->generation = 0; /* permanent retirement */
    }
    return BH_DISPLAY_RESULT_OK;
}

/* Attach Buffer */
bh_display_result_t bh_compositor_attach_buffer(bh_compositor_core_t *core, bh_display_lease_handle_t lease_handle, bh_gui_surface_handle_t surface_handle, bh_gui_buffer_handle_t buffer_handle) {
    if (!bh_gui_handle_validate(surface_handle, BH_GUI_RESOURCE_SURFACE)) return BH_DISPLAY_RESULT_INVALID_ARGUMENT;
    uint16_t surf_slot = bh_gui_handle_get_slot(surface_handle);
    uint32_t surf_idx = surf_slot - 1;
    if (surf_idx >= MAX_SURFACES || !core->surfaces[surf_idx].active || core->surfaces[surf_idx].handle != surface_handle) {
        return BH_DISPLAY_RESULT_NOT_FOUND;
    }
    bh_surface_entry_t *surf = &core->surfaces[surf_idx];
    if (surf->lease_handle != lease_handle) return BH_DISPLAY_RESULT_PERMISSION_DENIED;

    if (buffer_handle != BH_GUI_HANDLE_INVALID) {
        if (!bh_gui_handle_validate(buffer_handle, BH_GUI_RESOURCE_BUFFER)) return BH_DISPLAY_RESULT_INVALID_ARGUMENT;
        uint16_t buf_slot = bh_gui_handle_get_slot(buffer_handle);
        uint32_t buf_idx = buf_slot - 1;
        if (buf_idx >= MAX_BUFFERS || !core->buffers[buf_idx].active || core->buffers[buf_idx].handle != buffer_handle) {
            return BH_DISPLAY_RESULT_NOT_FOUND;
        }
        bh_buffer_entry_t *buf = &core->buffers[buf_idx];
        if (buf->lease_handle != lease_handle) return BH_DISPLAY_RESULT_PERMISSION_DENIED;

        /* A buffer cannot be attached to multiple surfaces unless explicitly marked shareable */
        if (buf->attached_surface != BH_GUI_HANDLE_INVALID && buf->attached_surface != surface_handle) {
            return BH_DISPLAY_RESULT_BUSY;
        }

        /* Detach previously attached buffer on this surface */
        if (surf->attached_buffer != BH_GUI_HANDLE_INVALID && surf->attached_buffer != buffer_handle) {
            uint16_t prev_slot = bh_gui_handle_get_slot(surf->attached_buffer);
            uint32_t prev_idx = prev_slot - 1;
            if (prev_idx < MAX_BUFFERS && core->buffers[prev_idx].active && core->buffers[prev_idx].handle == surf->attached_buffer) {
                core->buffers[prev_idx].state = BH_BUFFER_STATE_ALLOCATED;
                core->buffers[prev_idx].attached_surface = BH_GUI_HANDLE_INVALID;
            }
        }

        surf->attached_buffer = buffer_handle;
        buf->attached_surface = surface_handle;
        buf->state = BH_BUFFER_STATE_ATTACHED;
    } else {
        /* Detach only */
        if (surf->attached_buffer != BH_GUI_HANDLE_INVALID) {
            uint16_t prev_slot = bh_gui_handle_get_slot(surf->attached_buffer);
            uint32_t prev_idx = prev_slot - 1;
            if (prev_idx < MAX_BUFFERS && core->buffers[prev_idx].active && core->buffers[prev_idx].handle == surf->attached_buffer) {
                core->buffers[prev_idx].state = BH_BUFFER_STATE_ALLOCATED;
                core->buffers[prev_idx].attached_surface = BH_GUI_HANDLE_INVALID;
            }
            surf->attached_buffer = BH_GUI_HANDLE_INVALID;
        }

        /* Force stop scanout of scanning/retired buffer if we are detaching */
        if (surf->scanning_buffer != BH_GUI_HANDLE_INVALID) {
            uint16_t prev_slot = bh_gui_handle_get_slot(surf->scanning_buffer);
            uint32_t prev_idx = prev_slot - 1;
            if (prev_idx < MAX_BUFFERS && core->buffers[prev_idx].active && core->buffers[prev_idx].handle == surf->scanning_buffer) {
                core->buffers[prev_idx].state = BH_BUFFER_STATE_RELEASED;
                core->buffers[prev_idx].attached_surface = BH_GUI_HANDLE_INVALID;
            }
            surf->scanning_buffer = BH_GUI_HANDLE_INVALID;
        }
    }

    if (surf->state == BH_SURFACE_STATE_CREATED) {
        surf->state = BH_SURFACE_STATE_CONFIGURED;
    }
    return BH_DISPLAY_RESULT_OK;
}

/* Software Fence Operations */
bh_display_result_t bh_compositor_create_fence(bh_compositor_core_t *core, bh_display_lease_handle_t lease_handle, bool is_release, bh_gui_fence_handle_t *out_fence) {
    uint32_t found_idx = UINT32_MAX;
    for (uint32_t i = 0; i < MAX_FENCES; i++) {
        if (!core->fences[i].active) {
            found_idx = i;
            break;
        }
    }
    if (found_idx == UINT32_MAX) return BH_DISPLAY_RESULT_NO_RESOURCES;

    bh_fence_entry_t *fence = &core->fences[found_idx];
    fence->active = true;
    fence->lease_handle = lease_handle;
    fence->state = BH_FENCE_STATE_UNSIGNALED;
    fence->is_release_fence = is_release;

    fence->handle = bh_gui_handle_pack(found_idx + 1, fence->generation, BH_GUI_RESOURCE_FENCE, BH_GUI_HANDLE_ABI_V1);
    if (out_fence) *out_fence = fence->handle;
    return BH_DISPLAY_RESULT_OK;
}

bh_display_result_t bh_compositor_signal_fence(bh_compositor_core_t *core, bh_gui_fence_handle_t fence_handle) {
    if (!bh_gui_handle_validate(fence_handle, BH_GUI_RESOURCE_FENCE)) return BH_DISPLAY_RESULT_INVALID_ARGUMENT;
    uint16_t slot = bh_gui_handle_get_slot(fence_handle);
    uint32_t idx = slot - 1;
    if (idx >= MAX_FENCES || !core->fences[idx].active || core->fences[idx].handle != fence_handle) {
        return BH_DISPLAY_RESULT_NOT_FOUND;
    }

    core->fences[idx].state = BH_FENCE_STATE_SIGNALED;
    return BH_DISPLAY_RESULT_OK;
}

bh_display_result_t bh_compositor_wait_fence(bh_compositor_core_t *core, bh_gui_fence_handle_t fence_handle, bh_monotonic_deadline_ns_t deadline) {
    if (fence_handle == BH_GUI_HANDLE_INVALID) return BH_DISPLAY_RESULT_OK;

    if (!bh_gui_handle_validate(fence_handle, BH_GUI_RESOURCE_FENCE)) return BH_DISPLAY_RESULT_INVALID_ARGUMENT;
    uint16_t slot = bh_gui_handle_get_slot(fence_handle);
    uint32_t idx = slot - 1;
    if (idx >= MAX_FENCES || !core->fences[idx].active || core->fences[idx].handle != fence_handle) {
        return BH_DISPLAY_RESULT_NOT_FOUND;
    }

    bh_fence_entry_t *fence = &core->fences[idx];
    if (fence->state == BH_FENCE_STATE_SIGNALED) return BH_DISPLAY_RESULT_OK;

    if (deadline == 0) {
        /* Non-blocking. If not ready, return fence unsignaled */
        return BH_DISPLAY_RESULT_FENCE_UNSIGNALED;
    }

    /* Simulate bounded deadline wait with injected clock */
    uint64_t current_time = 0;
    if (core->clock.now_ns) {
        current_time = core->clock.now_ns(core->clock.ctx);
    }
    if (deadline <= current_time) {
        fence->state = BH_FENCE_STATE_TIMED_OUT;
        return BH_DISPLAY_RESULT_TIMEOUT;
    }

    /* In a mock / fake clock environment, wait_fence might succeed if signaled or fail if deadline elapsed */
    /* For deterministic host testing, let's keep it as: if clock is advanced, wait resolves */
    if (fence->state == BH_FENCE_STATE_SIGNALED) {
        return BH_DISPLAY_RESULT_OK;
    }

    return BH_DISPLAY_RESULT_FENCE_UNSIGNALED;
}

/* Direct-Scanout Evaluator */
bh_direct_scanout_reason_t bh_compositor_evaluate_direct_scanout(bh_compositor_core_t *core, bh_display_entry_t *disp, bh_surface_entry_t *surf, bh_buffer_entry_t *buf) {
    /* 1. Exactly one visible full-screen surface exists */
    uint32_t visible_count = 0;
    for (uint32_t i = 0; i < MAX_SURFACES; i++) {
        if (core->surfaces[i].active && core->surfaces[i].state == BH_SURFACE_STATE_VISIBLE) {
            visible_count++;
        }
    }
    if (visible_count > 1) {
        return BH_SCANOUT_REASON_MULTIPLE_VISIBLE_SURFACES;
    }

    /* Is it fullscreen? */
    if (surf->width != disp->width || surf->height != disp->height) {
        return BH_SCANOUT_REASON_NOT_FULLSCREEN;
    }

    /* Stride and byte-size checks */
    if (buf->desc.width != disp->width || buf->desc.height != disp->height) {
        return BH_SCANOUT_REASON_SCALING_UNSUPPORTED;
    }

    /* Usage check */
    if (!(buf->desc.usage_flags & BH_DISPLAY_BUFFER_USAGE_SCANOUT)) {
        return BH_SCANOUT_REASON_USAGE_MISSING;
    }

    /* Check plane compatibility */
    bool plane_found = false;
    for (uint32_t p = 0; p < disp->plane_count; p++) {
        const bh_plane_desc_t *plane = &disp->planes[p];
        if (!plane->direct_scanout_support) continue;

        /* Format support mask */
        uint32_t fmt_bit = 0;
        if (buf->desc.pixel_format == BH_DISPLAY_FORMAT_XRGB8888) fmt_bit = 1 << 0;
        else if (buf->desc.pixel_format == BH_DISPLAY_FORMAT_ARGB8888) fmt_bit = 1 << 1;
        else if (buf->desc.pixel_format == BH_DISPLAY_FORMAT_RGB565) fmt_bit = 1 << 2;
        else if (buf->desc.pixel_format == BH_DISPLAY_FORMAT_NV12) fmt_bit = 1 << 3;

        if (!(plane->supported_formats_mask & fmt_bit)) continue;

        /* Size constraints */
        if (buf->desc.width < plane->min_width || buf->desc.width > plane->max_width) continue;
        if (buf->desc.height < plane->min_height || buf->desc.height > plane->max_height) continue;

        plane_found = true;
        break;
    }

    if (!plane_found) {
        return BH_SCANOUT_REASON_NO_COMPATIBLE_PLANE;
    }

    return BH_SCANOUT_REASON_ELIGIBLE;
}

/* Present Surface Transaction */
bh_display_result_t bh_compositor_present(bh_compositor_core_t *core, bh_display_lease_handle_t lease_handle, bh_gui_surface_handle_t surface_handle, bh_gui_buffer_handle_t buffer_handle, bh_gui_fence_handle_t acquire_fence, bh_monotonic_deadline_ns_t deadline, bh_direct_scanout_reason_t *out_reason, bh_gui_fence_handle_t *out_release_fence) {
    if (!core->profile_caps.ui_enabled) return BH_DISPLAY_RESULT_PROFILE_UNSUPPORTED;

    /* 1. VALIDATE_CLIENT & VALIDATE_LEASE */
    if (!bh_gui_handle_validate(lease_handle, BH_GUI_RESOURCE_LEASE)) return BH_DISPLAY_RESULT_INVALID_ARGUMENT;
    uint16_t lease_slot = bh_gui_handle_get_slot(lease_handle);
    uint32_t lease_idx = lease_slot - 1;
    if (lease_idx >= MAX_LEASES || !core->leases[lease_idx].active || core->leases[lease_idx].handle != lease_handle) {
        return BH_DISPLAY_RESULT_NOT_FOUND;
    }
    bh_lease_entry_t *lease = &core->leases[lease_idx];
    if (lease->state == BHARAT_DISPLAY_LEASE_STATE_REVOKED) return BH_DISPLAY_RESULT_REVOKED;
    if (lease->state == BHARAT_DISPLAY_LEASE_STATE_REVOKING) {
        /* If revoked or currently revoking, check if deadline has expired */
        if (core->clock.now_ns) {
            if (core->clock.now_ns(core->clock.ctx) >= lease->revocation_deadline) {
                lease->state = BHARAT_DISPLAY_LEASE_STATE_REVOKED;
                return BH_DISPLAY_RESULT_REVOKED;
            }
        }
    }

    if (!(lease->rights & BHARAT_DISPLAY_RIGHT_PRESENT)) {
        return BH_DISPLAY_RESULT_PERMISSION_DENIED;
    }

    /* 2. VALIDATE_SURFACE */
    if (!bh_gui_handle_validate(surface_handle, BH_GUI_RESOURCE_SURFACE)) return BH_DISPLAY_RESULT_INVALID_ARGUMENT;
    uint16_t surf_slot = bh_gui_handle_get_slot(surface_handle);
    uint32_t surf_idx = surf_slot - 1;
    if (surf_idx >= MAX_SURFACES || !core->surfaces[surf_idx].active || core->surfaces[surf_idx].handle != surface_handle) {
        return BH_DISPLAY_RESULT_NOT_FOUND;
    }
    bh_surface_entry_t *surf = &core->surfaces[surf_idx];
    if (surf->lease_handle != lease_handle) return BH_DISPLAY_RESULT_PERMISSION_DENIED;
    if (surf->state == BH_SURFACE_STATE_DESTROYED || surf->state == BH_SURFACE_STATE_DESTROY_PENDING) {
        return BH_DISPLAY_RESULT_BAD_STATE;
    }

    /* 3. VALIDATE_BUFFER */
    if (!bh_gui_handle_validate(buffer_handle, BH_GUI_RESOURCE_BUFFER)) return BH_DISPLAY_RESULT_INVALID_ARGUMENT;
    uint16_t buf_slot = bh_gui_handle_get_slot(buffer_handle);
    uint32_t buf_idx = buf_slot - 1;
    if (buf_idx >= MAX_BUFFERS || !core->buffers[buf_idx].active || core->buffers[buf_idx].handle != buffer_handle) {
        return BH_DISPLAY_RESULT_NOT_FOUND;
    }
    bh_buffer_entry_t *buf = &core->buffers[buf_idx];
    if (buf->lease_handle != lease_handle) return BH_DISPLAY_RESULT_PERMISSION_DENIED;
    if (buf->state == BH_BUFFER_STATE_REVOKED) return BH_DISPLAY_RESULT_REVOKED;

    /* Buffer must be attached or previously attached to this surface */
    if (buf->attached_surface != surface_handle) {
        return BH_DISPLAY_RESULT_BAD_STATE;
    }

    /* Failure Injection before Commit */
    if (core->inject_queue_failure) {
        return BH_DISPLAY_RESULT_INTERNAL;
    }

    /* 4. WAIT_ACQUIRE_FENCE */
    if (acquire_fence != BH_GUI_HANDLE_INVALID) {
        bh_display_result_t wait_res = bh_compositor_wait_fence(core, acquire_fence, deadline);
        if (wait_res != BH_DISPLAY_RESULT_OK) {
            if (out_reason) *out_reason = BH_SCANOUT_REASON_ACQUIRE_FENCE_UNSIGNALED;
            return wait_res;
        }
    }

    /* 5. EVALUATE_PLANES & DIRECT SCANOUT DECISION */
    uint16_t disp_slot = bh_gui_handle_get_slot(lease->display_handle);
    uint32_t disp_idx = disp_slot - 1;
    bh_display_entry_t *disp = &core->displays[disp_idx];

    bh_direct_scanout_reason_t scanout_reason = bh_compositor_evaluate_direct_scanout(core, disp, surf, buf);
    if (out_reason) *out_reason = scanout_reason;

    /* Fail close if core->inject_commit_failure is set */
    if (core->inject_commit_failure) {
        return BH_DISPLAY_RESULT_INTERNAL;
    }

    /* 6. QUEUE -> COMMIT */
    buf->state = BH_BUFFER_STATE_QUEUED;

    /* Transition of previous scanning buffer to retired */
    bh_gui_buffer_handle_t prev_scanning = surf->scanning_buffer;
    if (prev_scanning != BH_GUI_HANDLE_INVALID) {
        uint16_t prev_slot = bh_gui_handle_get_slot(prev_scanning);
        uint32_t prev_idx = prev_slot - 1;
        if (prev_idx < MAX_BUFFERS && core->buffers[prev_idx].active && core->buffers[prev_idx].handle == prev_scanning) {
            core->buffers[prev_idx].state = BH_BUFFER_STATE_RETIRED;

            /* Signal the previous buffer's release fence if any */
            if (core->buffers[prev_idx].release_fence != BH_GUI_HANDLE_INVALID) {
                bh_compositor_signal_fence(core, core->buffers[prev_idx].release_fence);
            }
        }
        surf->retired_buffer = prev_scanning;
    }

    /* Commit new scanning buffer */
    surf->scanning_buffer = buffer_handle;
    buf->state = BH_BUFFER_STATE_SCANNING_OUT;
    surf->state = BH_SURFACE_STATE_VISIBLE;
    surf->frame_counter++;

    /* Allocate and associate a release fence if requested */
    if (out_release_fence) {
        bh_gui_fence_handle_t rel_fence;
        bh_display_result_t f_res = bh_compositor_create_fence(core, lease_handle, true, &rel_fence);
        if (f_res == BH_DISPLAY_RESULT_OK) {
            buf->release_fence = rel_fence;
            *out_release_fence = rel_fence;
        } else {
            buf->release_fence = BH_GUI_HANDLE_INVALID;
            *out_release_fence = BH_GUI_HANDLE_INVALID;
        }
    }

    /* Update metrics */
    core->frame_count++;
    if (scanout_reason == BH_SCANOUT_REASON_ELIGIBLE) {
        core->direct_scanout_count++;
    } else {
        core->composition_count++;
    }

    return BH_DISPLAY_RESULT_OK;
}

/* Retire Presentation */
bh_display_result_t bh_compositor_retire_presentation(bh_compositor_core_t *core, bh_display_lease_handle_t lease_handle, bh_gui_surface_handle_t surface_handle) {
    if (!bh_gui_handle_validate(surface_handle, BH_GUI_RESOURCE_SURFACE)) return BH_DISPLAY_RESULT_INVALID_ARGUMENT;
    uint16_t slot = bh_gui_handle_get_slot(surface_handle);
    uint32_t idx = slot - 1;
    if (idx >= MAX_SURFACES || !core->surfaces[idx].active || core->surfaces[idx].handle != surface_handle) {
        return BH_DISPLAY_RESULT_NOT_FOUND;
    }

    bh_surface_entry_t *surf = &core->surfaces[idx];
    if (surf->lease_handle != lease_handle) return BH_DISPLAY_RESULT_PERMISSION_DENIED;

    if (core->inject_retire_failure) {
        return BH_DISPLAY_RESULT_INTERNAL;
    }

    /* Move retired buffer to released */
    bh_gui_buffer_handle_t prev_retired = surf->retired_buffer;
    if (prev_retired != BH_GUI_HANDLE_INVALID) {
        uint16_t prev_slot = bh_gui_handle_get_slot(prev_retired);
        uint32_t prev_idx = prev_slot - 1;
        if (prev_idx < MAX_BUFFERS && core->buffers[prev_idx].active && core->buffers[prev_idx].handle == prev_retired) {
            core->buffers[prev_idx].state = BH_BUFFER_STATE_RELEASED;
        }
        surf->retired_buffer = BH_GUI_HANDLE_INVALID;
    }

    /* If surface is in DESTROY_PENDING, we can now retire the scanning buffer as well */
    if (surf->state == BH_SURFACE_STATE_DESTROY_PENDING) {
        bh_gui_buffer_handle_t scanning = surf->scanning_buffer;
        if (scanning != BH_GUI_HANDLE_INVALID) {
            uint16_t scan_slot = bh_gui_handle_get_slot(scanning);
            uint32_t scan_idx = scan_slot - 1;
            if (scan_idx < MAX_BUFFERS && core->buffers[scan_idx].active && core->buffers[scan_idx].handle == scanning) {
                core->buffers[scan_idx].state = BH_BUFFER_STATE_RELEASED;

                /* Signal release fence */
                if (core->buffers[scan_idx].release_fence != BH_GUI_HANDLE_INVALID) {
                    bh_compositor_signal_fence(core, core->buffers[scan_idx].release_fence);
                }
            }
            surf->scanning_buffer = BH_GUI_HANDLE_INVALID;
        }

        /* Complete destruction */
        surf->state = BH_SURFACE_STATE_DESTROYED;
        surf->active = false;
        if (surf->generation < UINT32_MAX) {
            surf->generation++;
        } else {
            surf->generation = 0;
        }
    }

    core->retirement_count++;
    return BH_DISPLAY_RESULT_OK;
}

/* Query Presentation Status */
bh_display_result_t bh_compositor_query_presentation_status(bh_compositor_core_t *core, bh_display_lease_handle_t lease_handle, bh_gui_surface_handle_t surface_handle, uint32_t *out_state, bh_gui_buffer_handle_t *out_active_buffer, uint64_t *out_frame_counter) {
    if (!bh_gui_handle_validate(surface_handle, BH_GUI_RESOURCE_SURFACE)) return BH_DISPLAY_RESULT_INVALID_ARGUMENT;
    uint16_t slot = bh_gui_handle_get_slot(surface_handle);
    uint32_t idx = slot - 1;
    if (idx >= MAX_SURFACES || !core->surfaces[idx].active || core->surfaces[idx].handle != surface_handle) {
        return BH_DISPLAY_RESULT_NOT_FOUND;
    }

    bh_surface_entry_t *surf = &core->surfaces[idx];
    if (surf->lease_handle != lease_handle) return BH_DISPLAY_RESULT_PERMISSION_DENIED;

    if (out_state) *out_state = surf->state;
    if (out_active_buffer) *out_active_buffer = surf->scanning_buffer;
    if (out_frame_counter) *out_frame_counter = surf->frame_counter;

    return BH_DISPLAY_RESULT_OK;
}
