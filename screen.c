#include "screen.h"
#include "graphics.h"
#include "font.h"
#include "cursor.h"
#include "term_io.h"
#include "libc.h"    // <-- add this
// Screen dimensions in characters
#define COLS 128
#define ROWS 48

// Colors
#define COLOR_FG 0x00FFFFFF  // white text
#define COLOR_BG 0x00001133  // dark blue background

static int cur_col = 0;
static int cur_row = 0;
static int terminal_output_mode = 0;  // When 1, output goes to terminal buffer

void screen_set_terminal_output(int enabled) {
    terminal_output_mode = enabled;
}

// Convert character cell to pixel coordinates
static int cell_x(int col) { return col * font_char_width(); }
static int cell_y(int row) { return row * font_char_height(); }

void clear_screen(void) {
    fill_screen(COLOR_BG);
    cur_col = 0;
    cur_row = 0;
}

static void scroll_up(void) {
    int cw        = font_char_width();
    int ch        = font_char_height();
    uint32_t* fb  = fb_get();
    uint32_t  w   = fb_width_get();
    uint32_t  h   = fb_height_get();

    uint32_t copy_count = w * (h - ch);
    for (uint32_t i = 0; i < copy_count; i++) {
        fb[i] = fb[i + w * ch];
    }
    fill_rect(0, h - ch, w, ch, COLOR_BG);
    (void)cw;
}


void put_char(char c) {
    // Only redirect if terminal output mode is explicitly on
    if (terminal_output_mode) {
        term_io_putchar(c);
        return;
    }
    
    int cw = font_char_width();
    int ch = font_char_height();
    int screen_w = fb_width_get();
    int screen_h = fb_height_get();

    if (c == '\n') {
        cur_col = 0;
        cur_row++;
    } else if (c == '\b') {
        if (cur_col > 0) {
            cur_col--;
            fill_rect(cell_x(cur_col), cell_y(cur_row), cw, ch, COLOR_BG);
        }
    } else {
        font_draw_char(cell_x(cur_col), cell_y(cur_row), c, COLOR_FG, COLOR_BG);
        cur_col++;
        if (cur_col * cw >= (int)screen_w) {
            cur_col = 0;
            cur_row++;
        }
    }

    if (cur_row * ch >= (int)screen_h) {
        scroll_up();
        cur_row--;
    }

    update_cursor(cur_row, cur_col);
    (void)screen_h;
}

void print(const char* str) {
    if (terminal_output_mode) {
        while (*str) term_io_putchar(*str++);
        return;
    }
    while (*str) put_char(*str++);
}

// These are used by brickwm/editor - keep them working
void screen_put_char_at(int row, int col, char c, char color) {
    (void)color;
    font_draw_char(cell_x(col), cell_y(row), c, COLOR_FG, COLOR_BG);
}

char screen_get_char_at(int row, int col) {
    (void)row; (void)col;
    return ' ';
}

void get_cursor_position(int* row, int* col) {
    *row = cur_row;
    *col = cur_col;
}

typedef __builtin_va_list va_list;
#define va_start(ap, last) __builtin_va_start(ap, last)
#define va_end(ap)         __builtin_va_end(ap)

void print_fmt(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    char buf[512];
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    print(buf);
}