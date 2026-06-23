#include <stddef.h>
#include "progman.h"
#include "graphics.h"
#include "font.h"
#include "brickwm.h"
#include "term.h"
#include "editor.h"

#define ICON_W 80
#define ICON_H 80
#define ICONS_PER_ROW 5

typedef struct {
    const char* name;
    const char* cmd;
    int x, y;
    void (*launch)(void);
} ProgramIcon;

static void launch_terminal(void) {
    terminal_run();
}

static void launch_fileman(void) {
    extern void fileman_run(int, int, int, int);
    fileman_run(200, 100, 500, 380);
}

static void launch_editor(void) {
    editor_open("notes.txt");
}

static void launch_doom(void) {
    extern void doom_run(void);
    doom_run();
}

static ProgramIcon icons[] = {
    {"Terminal", "term", 0, 0, launch_terminal},
    {"Files",    "fm",   0, 0, launch_fileman},
    {"Editor",   "edit", 0, 0, launch_editor},
    {"DOOM",     "doom", 0, 0, launch_doom},
};

static int icon_count = sizeof(icons) / sizeof(icons[0]);

static int text_len(const char* s) {
    int n = 0;
    while (s[n]) n++;
    return n;
}

static void draw_terminal_icon(int x, int y) {
    fill_rect(x + 17, y + 12, 46, 34, 0x050505);
    draw_rect_outline(x + 17, y + 12, 46, 34, 0x66DD88);
    font_draw_string(x + 24, y + 22, ">_", 0x66DD88, 0x050505);
}

static void draw_files_icon(int x, int y) {
    fill_rect(x + 16, y + 18, 48, 32, 0xD6A84A);
    fill_rect(x + 16, y + 12, 24, 10, 0xF0C46B);
    draw_rect_outline(x + 16, y + 18, 48, 32, 0x7A5420);
}

static void draw_editor_icon(int x, int y) {
    fill_rect(x + 21, y + 10, 34, 44, 0xEEEEEE);
    draw_rect_outline(x + 21, y + 10, 34, 44, 0x333333);
    fill_rect(x + 27, y + 20, 22, 2, 0x333333);
    fill_rect(x + 27, y + 28, 22, 2, 0x333333);
    fill_rect(x + 27, y + 36, 16, 2, 0x333333);
    fill_rect(x + 51, y + 38, 14, 5, 0xF0C000);
    fill_rect(x + 62, y + 36, 4, 9, 0x333333);
}

static void draw_doom_icon(int x, int y) {
    fill_rect(x + 16, y + 14, 48, 34, 0x601010);
    draw_rect_outline(x + 16, y + 14, 48, 34, 0xD05030);
    fill_rect(x + 25, y + 24, 30, 12, 0x202020);
    fill_rect(x + 36, y + 18, 8, 24, 0xC0C0C0);
    fill_rect(x + 31, y + 37, 18, 8, 0x505050);
}

static void draw_icon_graphic(int idx, int x, int y) {
    if (idx == 0) draw_terminal_icon(x, y);
    else if (idx == 1) draw_files_icon(x, y);
    else if (idx == 2) draw_editor_icon(x, y);
    else if (idx == 3) draw_doom_icon(x, y);
}

static int find_icon_click(int mx, int my) {
    for (int i = 0; i < icon_count; i++) {
        if (mx >= icons[i].x && mx < icons[i].x + ICON_W &&
            my >= icons[i].y && my < icons[i].y + ICON_H) {
            return i;
        }
    }
    return -1;
}

static void draw_progman(int x, int y, int w, int h) {
    fill_rect(x, y, w, h, 0x114455);

    int start_x = x + 20;
    int start_y = y + 20;
    for (int i = 0; i < icon_count; i++) {
        int col = i % ICONS_PER_ROW;
        int row = i / ICONS_PER_ROW;
        icons[i].x = start_x + col * ICON_W + 10;
        icons[i].y = start_y + row * ICON_H + 10;

        fill_rect(icons[i].x, icons[i].y, ICON_W, ICON_H, 0x1B2A31);
        draw_rect_outline(icons[i].x, icons[i].y, ICON_W, ICON_H, 0x8DB9C7);
        draw_icon_graphic(i, icons[i].x, icons[i].y);

        int label_x = icons[i].x + (ICON_W - text_len(icons[i].name) * font_char_width()) / 2;
        if (label_x < icons[i].x + 2) label_x = icons[i].x + 2;
        font_draw_string(label_x, icons[i].y + 60, icons[i].name, 0xFFFFFF, 0x1B2A31);
    }
}

void progman_handle_click(int mx, int my) {
    int clicked = find_icon_click(mx, my);
    if (clicked >= 0 && clicked < icon_count && icons[clicked].launch) {
        icons[clicked].launch();
    }
}

void progman_run(void) {
    brickwm_init();
    brickwm_create_window("Program Manager", 100, 50, 500, 400, draw_progman, NULL);
    brickwm_run();
}
