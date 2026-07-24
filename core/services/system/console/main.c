#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <bharat/ipc/ipc.h>
#include <bharat/cap/cap.h>
#include "bharat/uapi/display/lease.h"
#include "bharat/runtime/runtime.h"
#include "bharat/uapi/ipc/status.h"
#include "font_8x16.h"

#define DISPLAY_BROKER_ENDPOINT 15

typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t cursor_x;
    uint32_t cursor_y;
    uint32_t *fb_pixels;
    bharat_display_lease_id_t lease_id;
    bool active;
} fb_text_backend_t;

static fb_text_backend_t g_fb_backend;

static void fb_render_char(char c, uint32_t x, uint32_t y, uint32_t color) {
    if (!g_fb_backend.fb_pixels || !g_fb_backend.active) return;

    const uint8_t *glyph = g_console_font_8x16[(uint8_t)c];
    // Fallback for unsupported glyphs
    if (glyph[0] == 0 && glyph[1] == 0 && glyph[2] == 0 && c != ' ' && c != 0) {
        glyph = g_console_font_8x16[(uint8_t)'?'];
    }

    // Bounds check for character position
    if ((x + 1) * 8 > g_fb_backend.width || (y + 1) * 16 > g_fb_backend.height) return;

    for (int row = 0; row < 16; row++) {
        for (int col = 0; col < 8; col++) {
            if (glyph[row] & (0x80 >> col)) {
                uint32_t px = (y * 16 + row) * g_fb_backend.width + (x * 8 + col);
                g_fb_backend.fb_pixels[px] = color;
            }
        }
    }
}

static void console_write_fb(const char *buf, size_t len) {
    if (!g_fb_backend.active) return;

    // Truncate overly long writes to avoid blocking service for too long
    if (len > 4096) len = 4096;

    for (size_t i = 0; i < len; i++) {
        if (buf[i] == '\n') {
            g_fb_backend.cursor_x = 0;
            g_fb_backend.cursor_y++;
        } else if (buf[i] == '\r') {
            g_fb_backend.cursor_x = 0;
        } else {
            fb_render_char(buf[i], g_fb_backend.cursor_x, g_fb_backend.cursor_y, 0xFFFFFFFF);
            g_fb_backend.cursor_x++;
            if ((g_fb_backend.cursor_x + 1) * 8 > g_fb_backend.width) {
                g_fb_backend.cursor_x = 0;
                g_fb_backend.cursor_y++;
            }
        }

        if ((g_fb_backend.cursor_y + 1) * 16 > g_fb_backend.height) {
            // Simple wrap around
            g_fb_backend.cursor_y = 0;
        }
    }
}

static void console_init_fb(void) {
    struct { uint32_t display_id; uint32_t requested_rights; } req;
    struct { uint32_t status; uint32_t lease_id; uint32_t granted_rights; uint64_t fb_ptr; } resp;

    req.display_id = 1;
    req.requested_rights = BHARAT_DISPLAY_RIGHT_LEASE | BHARAT_DISPLAY_RIGHT_WRITE | BHARAT_DISPLAY_RIGHT_PRESENT;

    bharat_ipc_msg_header_t req_hdr = { .opcode = 1, .payload_size = sizeof(req) };
    bharat_ipc_msg_header_t resp_hdr;

    if (bharat_ipc_call(DISPLAY_BROKER_ENDPOINT, &req_hdr, &req, &resp_hdr, &resp, sizeof(resp)) == 0 && resp.status == BHARAT_STATUS_OK) {
        g_fb_backend.lease_id = resp.lease_id;
        g_fb_backend.width = 800;
        g_fb_backend.height = 480;
        g_fb_backend.fb_pixels = (void*)resp.fb_ptr;
        g_fb_backend.cursor_x = 0;
        g_fb_backend.cursor_y = 0;
        g_fb_backend.active = true;
        bharat_runtime_log("Console FB backend initialized");
    } else {
        g_fb_backend.active = false;
        bharat_runtime_log("Console FB backend unavailable, falling back to UART only");
    }
}

static void send_reply(bharat_cap_handle_t target, bharat_ipc_msg_header_t *orig_hdr, bharat_status_t status) {
    bharat_ipc_msg_header_t reply_hdr = *orig_hdr;
    reply_hdr.flags |= BHARAT_IPC_FLAG_REPLY;
    reply_hdr.status = status;
    reply_hdr.payload_size = 0;
    bharat_ipc_send(target, &reply_hdr, NULL);
}

int main(int argc, char** argv) {
    (void)argc; (void)argv;
    bharat_runtime_log("Console service starting");

    // Framebuffer is optional
    console_init_fb();

    bharat_ipc_msg_header_t hdr;
    uint8_t payload[4096];

    while (true) {
        int32_t ret = bharat_ipc_recv(BHARAT_CAP_INVALID_HANDLE, &hdr, payload, sizeof(payload));
        if (ret == 0) {
            switch (hdr.opcode) {
                case 2: // WriteStream
                    // Multiplexed output
                    // 1. UART (stubbed)
                    // 2. FB
                    console_write_fb((char*)payload, hdr.payload_size);
                    send_reply(hdr.reply_endpoint, &hdr, BHARAT_STATUS_OK);
                    break;
                case 4: // OpenSession
                    // In real system, validate rights and create session object
                    send_reply(hdr.reply_endpoint, &hdr, BHARAT_STATUS_OK);
                    break;
                default:
                    send_reply(hdr.reply_endpoint, &hdr, BHARAT_IPC_STATUS_ERR_OPCODE);
                    break;
            }
        }
    }
    return 0;
}
