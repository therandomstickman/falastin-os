#include "fileman.h"
#include "graphics.h"
#include "font.h"
#include "fs.h"
#include "diskfs.h"
#include "libc.h"      // <-- add this, has snprintf
#include "malloc.h"
#include "screen.h"
#include "brickwm.h"
#include "keyboard.h"

static char dbg_buffer[32] = "K=??";

#define MAX_ENTRIES  128
#define ENTRY_H      18
#define SCROLL_SPEED 3

// Colors
#define COL_BG         0x001A1A2E
#define COL_SELECTED   0x00162447
#define COL_HIGHLIGHT  0x000F3460
#define COL_DIR        0x004488FF
#define COL_FILE       0x00CCCCCC
#define COL_TOOLBAR    0x00111122
#define COL_STATUS     0x00111122
#define COL_BORDER     0x00334466

static Entry  entries[MAX_ENTRIES];
static int    entry_count  = 0;
static int    selected     = 0;
static int    scroll_off   = 0;
static int    win_x, win_y, win_w, win_h;
static int    fm_handle    = -1;

// Input state for rename/new file
static int    input_mode   = 0;  // 0=normal, 1=new file, 2=new dir, 3=rename
static char   input_buf[32];
static int    input_len    = 0;

static void refresh_entries(void) {
    entry_count = 0;
    selected    = 0;
    scroll_off  = 0;

    // Use diskfs directly to list current dir
    // We'll read via fs_list by reimplementing a simple version
    // that fills our entries array
    //extern int diskfs_get_entries(Entry* out, int max);
    entry_count = diskfs_get_entries(entries, MAX_ENTRIES);
}

static void draw_toolbar(int x, int y, int w) {
    fill_rect(x, y, w, 24, COL_TOOLBAR);

    // Buttons
    struct { const char* label; int x; } buttons[] = {
        {"[New File]", 4},
        {"[New Dir]",  84},
        {"[Delete]",   164},
        {"[Rename]",   234},
    };
    for (int i = 0; i < 4; i++) {
        font_draw_string(x + buttons[i].x, y + 6,
                         buttons[i].label, 0x00AACCFF, COL_TOOLBAR);
    }
}

static void draw_status(int x, int y, int w, int h) {
    fill_rect(x, y + h - 18, w, 18, COL_STATUS);
    
    // Build status string manually instead of snprintf
    char status[128];
    int pos = 0;
    
    // "  X items   Selected: name"
    status[pos++] = ' ';
    status[pos++] = ' ';
    
    // item count
    char numbuf[16];
    itoa(entry_count, numbuf, 10);
    for (int i = 0; numbuf[i]; i++) status[pos++] = numbuf[i];
    
    const char* mid = " items   Selected: ";
    for (int i = 0; mid[i]; i++) status[pos++] = mid[i];
    
    // selected name
    const char* selname = entry_count > 0 ? entries[selected].name : "(none)";
    for (int i = 0; selname[i]; i++) status[pos++] = selname[i];
    
    status[pos] = '\0';
    
    font_draw_string(x + 4, y + h - 14, status, 0x00778899, COL_STATUS);
}

void fileman_draw(int x, int y, int w, int h) {
    win_x = x; win_y = y; win_w = w; win_h = h;

    // Background
    fill_rect(x, y, w, h, COL_BG);

    // Toolbar
    draw_toolbar(x, y, w);

    // File list area
    int list_y = y + 26;
    int list_h = h - 44;
    int visible = list_h / ENTRY_H;

    // Border around list
    draw_rect_outline(x, list_y, w, list_h, COL_BORDER);

    for (int i = 0; i < visible && i + scroll_off < entry_count; i++) {
        int idx = i + scroll_off;
        int ey  = list_y + i * ENTRY_H;

        // Row background
        if (idx == selected)
            fill_rect(x + 1, ey, w - 2, ENTRY_H, COL_HIGHLIGHT);
        else if (i % 2 == 0)
            fill_rect(x + 1, ey, w - 2, ENTRY_H, COL_BG);
        else
            fill_rect(x + 1, ey, w - 2, ENTRY_H, 0x00151528);

        // Icon + name
        uint32_t col = entries[idx].is_dir ? COL_DIR : COL_FILE;
        const char* icon = entries[idx].is_dir ? "[D] " : "[F] ";
        font_draw_string(x + 8, ey + 3, icon, col,
                         idx == selected ? COL_HIGHLIGHT :
                         (i % 2 == 0 ? COL_BG : 0x00151528));
        font_draw_string(x + 40, ey + 3, entries[idx].name, col,
                         idx == selected ? COL_HIGHLIGHT :
                         (i % 2 == 0 ? COL_BG : 0x00151528));

        // Size (files only)
        if (!entries[idx].is_dir) {
            char sizebuf[16];
            int si = 0;
            itoa(entries[idx].size, sizebuf, 10);
            // append " B"
            int sl = strlen(sizebuf);
            sizebuf[sl] = ' '; sizebuf[sl+1] = 'B'; sizebuf[sl+2] = '\0';
            font_draw_string(x + w - 70, ey + 3, sizebuf,
                             0x00556677,
                             idx == selected ? COL_HIGHLIGHT :
                             (i % 2 == 0 ? COL_BG : 0x00151528));
        }
    }

    // Input prompt if active
    if (input_mode) {
        const char* prompt =
            input_mode == 1 ? "New file name: " :
            input_mode == 2 ? "New dir name:  " :
                              "Rename to:     ";
        int py = list_y + list_h - 20;
        fill_rect(x, py, w, 20, 0x00223344);
        font_draw_string(x + 4, py + 4, prompt, 0x00FFCC00, 0x00223344);
        font_draw_string(x + 130, py + 4, input_buf, 0x00FFFFFF, 0x00223344);
        // Cursor
        font_draw_string(x + 130 + strlen(input_buf) * 8, py + 4,
                         "_", 0x00FFCC00, 0x00223344);
    }

    draw_status(x, y, w, h);
    // Draw debug info
    
}

void fileman_keypress(char c) {
    // Rest of your function...
    int visible = (win_h - 44) / ENTRY_H;

    if (input_mode) {
        if (c == '\n') {
            input_buf[input_len] = '\0';
            if (input_len > 0) {
                if (input_mode == 1) fs_create(input_buf);
                else if (input_mode == 2) fs_mkdir(input_buf);
                else if (input_mode == 3) {
                    if (!entries[selected].is_dir) {
                        char buf[8192];
                        int bytes = fs_read(entries[selected].name, buf, sizeof(buf));
                        fs_delete(entries[selected].name);
                        if (bytes > 0) fs_write(input_buf, buf, bytes);
                        else fs_create(input_buf);
                    }
                }
            }
            input_mode = 0;
            input_len  = 0;
            input_buf[0] = '\0';
            refresh_entries();
        } else if (c == '\b') {
            if (input_len > 0) input_buf[--input_len] = '\0';
        } else if (c >= ' ' && c <= '~' && input_len < 31) {
            input_buf[input_len++] = c;
            input_buf[input_len]   = '\0';
        }
        return;
    }

    if (c == KEY_UP) {
        if (selected > 0) {
            selected--;
            if (selected < scroll_off) scroll_off--;
        }
    } else if (c == KEY_DOWN) {
        if (selected < entry_count - 1) {
            selected++;
            if (selected >= scroll_off + visible) scroll_off++;
        }
    } else if (c == '\n') {
        if (entry_count > 0 && entries[selected].is_dir) {
            fs_cd(entries[selected].name);
            refresh_entries();
        }
    } else if (c == 'u') {
        fs_cd("..");
        refresh_entries();
    } else if (c == 'n') {
        input_mode = 1;
        input_len  = 0;
        input_buf[0] = '\0';
    } else if (c == 'd') {
        input_mode = 2;
        input_len  = 0;
        input_buf[0] = '\0';
    } else if (c == 'r') {
        if (entry_count > 0) {
            input_mode = 3;
            input_len  = 0;
            for (int i = 0; entries[selected].name[i] && i < 31; i++) {
                input_buf[i] = entries[selected].name[i];
                input_len = i + 1;
            }
            input_buf[input_len] = '\0';
        }
    } else if (c == KEY_DELETE) {
        if (entry_count > 0) {
            fs_delete(entries[selected].name);
            refresh_entries();
        }
    }
}
void fileman_run(int x, int y, int w, int h) {
    refresh_entries();
    fm_handle = brickwm_create_window(
        "File Manager", x, y, w, h,
        fileman_draw, fileman_keypress
    );
    // Force a full redraw so the new window appears
    // and is visually on top
}