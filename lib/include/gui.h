#ifndef BHARAT_GUI_H
#define BHARAT_GUI_H

#include <stdint.h>
#include "hal/gpu.h"

/*
 * Bharat-OS Modular Windowing System & Compositor
 * 
 * Runs entirely in User-Space leveraging the GPU HAL for hardware acceleration.
 * Based on a Wayland-like display server paradigm.
 */

typedef struct {
    uint32_t win_id;
    int x;
    int y;
    uint32_t width;
    uint32_t height;
    
    // Z-Order index
    uint32_t z_index;
    
    // Shared memory handle between client and compositor
    void* shm_buffer;
} gui_window_t;

typedef struct {
    gpu_device_t* display_gpu;
    uint32_t screen_width;
    uint32_t screen_height;
    
    // Linked list or array of managed windows
    gui_window_t** active_windows;
    uint32_t window_count;
} gui_compositor_t;

// Initialize the root display server and take control of the primary GPU output
int gui_compositor_init(gui_compositor_t* compositor, gpu_device_t* gpu);

// Request a new window surface from the Compositor (Client Side API)
int gui_create_window(uint32_t width, uint32_t height, gui_window_t* out_win);

// Push a frame to the compositor queue
int gui_submit_frame(gui_window_t* win);

// Compositor main loop (Server Side)
void gui_compositor_render_pass(gui_compositor_t* compositor);

#endif // BHARAT_GUI_H
