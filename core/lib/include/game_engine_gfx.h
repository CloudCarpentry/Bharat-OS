#ifndef BHARAT_GAME_ENGINE_GPU_H
#define BHARAT_GAME_ENGINE_GPU_H

#include <stdint.h>
#include "hal/gpu.h"
#include "gui.h"

/*
 * Bharat-OS Low-Latency Graphics Pipeline
 * Direct-to-Metal API specifically catering to AAA Game Engines (Unreal, Unity, custom)
 * and avoiding the overhead of the standard window manager compositor.
 */

typedef struct {
    gpu_device_t* gpu;
    gui_window_t* target_window; // The compositor window surface to take over
    
    // Double/Triple buffering swapchains mapped physically
    void** swapchain_buffers_phys;
    uint32_t num_buffers;
    uint32_t current_buffer_idx;
    
    // Command queue for direct Vulkan/Metal style submission
    void* command_queue_ring;
} game_gfx_pipeline_t;

// Request exclusive low-latency takeover of a GUI window for a Game Engine
int gfx_request_pipeline(gui_window_t* win, uint32_t num_buffers, game_gfx_pipeline_t* out_pipeline);

// Submit a raw rendering command buffer directly to the GPU hardware scheduler bypassing the OS
int gfx_submit_direct_commands(game_gfx_pipeline_t* pipeline, void* cmd_buf, uint32_t size);

// Swap the presentation buffer (Vsync synchronous or immediate asynchronous)
int gfx_present_frame(game_gfx_pipeline_t* pipeline, int wait_vsync);

#endif // BHARAT_GAME_ENGINE_GPU_H
