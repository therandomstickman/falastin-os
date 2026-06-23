#include "brickwm.h"
#include "graphics.h"
#include "font.h"
#include "mouse.h"
#include "cursor_gfx.h"
#include "keyboard.h"
#include <stdio.h>
#include "lwip_port.h"

#define TITLEBAR_H 20
#define BORDER 2
#define MAX_WINDOWS 8

static Window windows[MAX_WINDOWS];
static int window_count = 0;
static int dragging = 0;
static int drag_win = -1;
static int drag_off_x = 0, drag_off_y = 0;
static int running = 1;

extern void progman_handle_click(int, int);
extern void lwip_poll(void);

void brickwm_init(void) {
    running = 1;
    for (int i = 0; i < MAX_WINDOWS; i++) {
        windows[i].use_count = 0;
        windows[i].draw = 0;
        windows[i].keypress = 0;
    }
    window_count = 0;
}

int brickwm_create_window(const char* title, int x, int y, int w, int h,
                          void (*draw)(int, int, int, int),
                          void (*keypress)(char)) {
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (!windows[i].use_count) {
            windows[i].x = x;
            windows[i].y = y;
            windows[i].w = w;
            windows[i].h = h;
            windows[i].active = 1;
            windows[i].draw = draw;
            windows[i].keypress = keypress;
            windows[i].use_count = 1;

            for (int j = 0; title[j] && j < 31; j++)
                windows[i].title[j] = title[j];
            windows[i].title[31] = '\0';

            // Deactivate all other windows
            for (int j = 0; j < MAX_WINDOWS; j++)
                if (j != i) windows[j].active = 0;

            window_count++;
            return i;
        }
    }
    return -1;
}

static void draw_window(int idx) {
    Window* w = &windows[idx];
    if (!w->use_count) return;

    // Shadow
    fill_rect(w->x + 3, w->y + 3, w->w, w->h + TITLEBAR_H, 0x000000);

    // Border
    draw_rect_outline(w->x - BORDER, w->y - BORDER,
                      w->w + BORDER * 2, w->h + TITLEBAR_H + BORDER * 2,
                      0x888888);

    // Title bar
    uint32_t tbcol = w->active ? 0x0D2E5A : 0x224466;
    fill_rect(w->x, w->y, w->w, TITLEBAR_H, tbcol);
    font_draw_string(w->x + 4, w->y + 4, w->title, 0xFFFFFF, tbcol);

    // Close button
    fill_rect(w->x + w->w - 18, w->y + 2, 14, 14, 0xCC2200);
    font_draw_string(w->x + w->w - 14, w->y + 4, "X", 0xFFFFFF, 0xCC2200);

    // Client area
    fill_rect(w->x, w->y + TITLEBAR_H, w->w, w->h, 0x000000);

    // Draw callback
    if (w->draw)
        w->draw(w->x, w->y + TITLEBAR_H, w->w, w->h);
}

static void draw_all(void) {
    cursor_gfx_erase();
    fill_screen(0x003366);
    for (int i = 0; i < MAX_WINDOWS; i++)
        if (windows[i].use_count)
            draw_window(i);
}

static void redraw_all_with_cursor(void) {
    draw_all();
    cursor_gfx_draw(mouse_x(), mouse_y());
}

static int title_equals(const char* a, const char* b) {
    while (*a && *b && *a == *b) {
        a++;
        b++;
    }
    return *a == *b;
}

static int focused_window(void) {
    // Return highest index active window with a keypress handler
    for (int i = MAX_WINDOWS - 1; i >= 0; i--)
        if (windows[i].use_count && windows[i].active && windows[i].keypress)
            return i;
    // Fall back to any window with a keypress handler
    for (int i = MAX_WINDOWS - 1; i >= 0; i--)
        if (windows[i].use_count && windows[i].keypress)
            return i;
    return -1;
}

static int find_window_at(int mx, int my) {
    for (int i = MAX_WINDOWS - 1; i >= 0; i--) {
        if (!windows[i].use_count) continue;
        Window* w = &windows[i];
        if (mx >= w->x && mx < w->x + w->w &&
            my >= w->y && my < w->y + w->h + TITLEBAR_H)
            return i;
    }
    return -1;
}

static int check_close_click(int mx, int my, int idx) {
    Window* w = &windows[idx];
    int cx = w->x + w->w - 18;
    int cy = w->y + 2;
    return mx >= cx && mx < cx + 14 && my >= cy && my < cy + 14;
}

static int check_titlebar_click(int mx, int my, int idx) {
    Window* w = &windows[idx];
    return mx >= w->x && mx < w->x + w->w - 18 &&
           my >= w->y && my < w->y + TITLEBAR_H;
}

static int check_client_click(int mx, int my, int idx) {
    Window* w = &windows[idx];
    return mx >= w->x && mx < w->x + w->w &&
           my >= w->y + TITLEBAR_H && my < w->y + w->h + TITLEBAR_H;
}

void brickwm_close_window(int handle) {
    if (handle < 0 || handle >= MAX_WINDOWS) return;
    if (!windows[handle].use_count) return;
    windows[handle].use_count = 0;
    windows[handle].draw = 0;
    windows[handle].keypress = 0;
    window_count--;

    // Focus the next available window
    for (int i = MAX_WINDOWS - 1; i >= 0; i--) {
        if (windows[i].use_count) {
            windows[i].active = 1;
            break;
        }
    }
    draw_all();
}

int brickwm_window_count(void) {
    return window_count;
}

void brickwm_run(void) {


    int last_mx = 0, last_my = 0;
    int last_buttons = 0;

    redraw_all_with_cursor();

    while (running && window_count > 0) {
        int mx = mouse_x();
        int my = mouse_y();
        int mb = mouse_buttons();

        int left_down     = (mb & 0x01) && !(last_buttons & 0x01);
        int left_held     = (mb & 0x01);
        int left_released = !(mb & 0x01) && (last_buttons & 0x01);
        int mouse_moved   = (mx != last_mx || my != last_my);
        lwip_poll();

        // Dragging
        if (dragging && left_held && mouse_moved && drag_win >= 0) {
            windows[drag_win].x = mx - drag_off_x;
            windows[drag_win].y = my - drag_off_y;
            redraw_all_with_cursor();
        }

        if (left_released && dragging) {
            dragging = 0;
            drag_win = -1;
        }

        // Click handling
        if (left_down) {
            int clicked = find_window_at(mx, my);

            if (clicked >= 0) {
                // Bring to front (make active)
                for (int i = 0; i < MAX_WINDOWS; i++)
                    windows[i].active = (i == clicked);

                if (check_close_click(mx, my, clicked)) {
                    brickwm_close_window(clicked);

                } else if (check_titlebar_click(mx, my, clicked)) {
                    dragging   = 1;
                    drag_win   = clicked;
                    drag_off_x = mx - windows[clicked].x;
                    drag_off_y = my - windows[clicked].y;
                    redraw_all_with_cursor();

                } else if (check_client_click(mx, my, clicked)) {
                    // Pass click coords only to the program manager window.
                    if (title_equals(windows[clicked].title, "Program Manager")) {
                        progman_handle_click(mx, my);
                    }
                    redraw_all_with_cursor();
                }
            }
        }

        // Keyboard — always goes to focused window
        if (keyboard_has_char()) {
            char c = keyboard_getchar();
            int fw = focused_window();
            
            if (fw >= 0) {
                cursor_gfx_erase();
                windows[fw].keypress(c);
                draw_window(fw);
                cursor_gfx_draw(mouse_x(), mouse_y());
            }
        }

        // Cursor update
        if (mouse_moved) {
            cursor_gfx_erase();
            cursor_gfx_draw(mx, my);
        }

        last_mx = mx;
        last_my = my;
        last_buttons = mb;
    }

    fill_screen(0x003366);
}
