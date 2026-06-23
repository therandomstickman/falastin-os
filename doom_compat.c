#include "doom_compat.h"
#include "fs.h"
#include "timer.h"
#include "mouse.h"
#include "keyboard.h"
#include "graphics.h"
#include "screen.h"
#include "libc.h"

// Framebuffer state
static uint32_t* fb = 0;
static int fb_w = 0;
static int fb_h = 0;
static int doom_running = 1;

// File descriptor table
typedef struct {
    char name[256];
    int used;
    int pos;
    int size;
} DoomFile;

static DoomFile doom_files[16];

void doom_video_init(void) {
    fb = fb_get();
    fb_w = fb_width_get();
    fb_h = fb_height_get();
    fill_screen(0x000000);
}

void doom_draw_pixel(int x, int y, uint32_t color) {
    if (fb && x >= 0 && x < fb_w && y >= 0 && y < fb_h) {
        fb[y * fb_w + x] = color;
    }
}

void doom_draw_rect(int x, int y, int w, int h, uint32_t color) {
    fill_rect(x, y, w, h, color);
}

void doom_flush(void) {
    // Framebuffer updates immediately
}

int doom_open(const char* path, int flags) {
    (void)flags;
    for (int i = 0; i < 16; i++) {
        if (!doom_files[i].used) {
            strcpy(doom_files[i].name, path);
            doom_files[i].used = 1;
            doom_files[i].pos = 0;
            
            // Get file size by reading into a dummy buffer
            char dummy[4096];
            int bytes = fs_read(path, dummy, sizeof(dummy));
            if (bytes > 0) {
                doom_files[i].size = bytes;
            } else {
                doom_files[i].size = 0;
                // Try to create file
                fs_create(path);
            }
            return i + 3; // Reserve 0,1,2 for stdin/out/err
        }
    }
    return -1;
}

int doom_read(int fd, void* buf, int count) {
    int idx = fd - 3;
    if (idx < 0 || idx >= 16 || !doom_files[idx].used) return -1;
    
    char* buffer = (char*)buf;
    int bytes = fs_read(doom_files[idx].name, buffer, count);
    if (bytes > 0) {
        doom_files[idx].pos += bytes;
    }
    return bytes;
}

int doom_write(int fd, const void* buf, int count) {
    // stdout/stderr -> console
    if (fd == 1 || fd == 2) {
        const char* str = (const char*)buf;
        for (int i = 0; i < count && str[i]; i++) {
            put_char(str[i]);
        }
        return count;
    }
    
    int idx = fd - 3;
    if (idx < 0 || idx >= 16 || !doom_files[idx].used) return -1;
    
    // Write to file (append)
    const char* data = (const char*)buf;
    fs_write(doom_files[idx].name, data, count);
    doom_files[idx].pos += count;
    return count;
}

int doom_close(int fd) {
    int idx = fd - 3;
    if (idx >= 0 && idx < 16) {
        doom_files[idx].used = 0;
        doom_files[idx].size = 0;
        doom_files[idx].pos = 0;
        doom_files[idx].name[0] = '\0';
    }
    return 0;
}

int doom_lseek(int fd, int offset, int whence) {
    int idx = fd - 3;
    if (idx < 0 || idx >= 16 || !doom_files[idx].used) return -1;
    
    switch (whence) {
        case 0: doom_files[idx].pos = offset; break; // SEEK_SET
        case 1: doom_files[idx].pos += offset; break; // SEEK_CUR
        case 2: doom_files[idx].pos = doom_files[idx].size + offset; break; // SEEK_END
        default: return -1;
    }
    return doom_files[idx].pos;
}

int doom_get_key(void) {
    if (keyboard_has_char()) {
        int c = keyboard_getchar();
        return c;
    }
    return 0;
}
void doom_get_mouse(int* dx, int* dy, int* buttons) {
    static int old_x = -1, old_y = -1;
    int x = mouse_x();
    int y = mouse_y();
    
    if (old_x == -1) {
        old_x = x;
        old_y = y;
    }
    
    *dx = x - old_x;
    *dy = y - old_y;
    *buttons = mouse_buttons();
    
    old_x = x;
    old_y = y;
}

uint32_t doom_get_ticks(void) {
    return timer_ticks();
}

void doom_sleep(uint32_t ms) {
    timer_sleep(ms);
}

// DOOM's main function (defined in doom source)
extern int doom_main(int argc, char** argv);

void doom_run(void) {
    doom_running = 1;
    
    print("=== DOOM for FalastinOS ===\n");
    print("Loading DOOM...\n");
    
    doom_video_init();
    
    print("Starting built-in Doom renderer...\n");
    print("Controls: arrows/WASD move, Q/E strafe, ESC exits.\n");
    
    // Prepare arguments
    char* argv[] = {"doom", 0};
    
    // Call DOOM main
    doom_main(1, argv);
    
    print("DOOM exited.\n");
}
