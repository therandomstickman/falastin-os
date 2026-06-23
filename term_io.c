#include "term_io.h"
#include "graphics.h"
#include "font.h"

char term_output[30][75];
int term_cursor_row = 0;
int term_cursor_col = 0;
static int term_active = 0;
static int needs_redraw = 1;
static int cursor_blink = 0;
static int blink_counter = 0;

void term_io_init(void) {
    for (int row = 0; row < 30; row++)
        for (int col = 0; col < 75; col++)
            term_output[row][col] = '\0';
    term_cursor_row = 0;
    term_cursor_col = 0;
    term_active = 1;   // make sure this is here
    needs_redraw = 1;
}

void term_io_set_needs_redraw(void) {
    needs_redraw = 1;
}

void term_io_putchar(char c) {
    if (!term_active) return;
    
    if (c == '\n') {
        term_cursor_row++;
        term_cursor_col = 0;
    } else if (c == '\b') {
        if (term_cursor_col > 0) {
            term_cursor_col--;
            term_output[term_cursor_row][term_cursor_col] = '\0';
        }
    } else if (c >= 32 && c <= 126) {
        term_output[term_cursor_row][term_cursor_col] = c;
        term_cursor_col++;
        if (term_cursor_col >= 75) {
            term_cursor_col = 0;
            term_cursor_row++;
        }
    }
    
    // Scroll if needed
    while (term_cursor_row >= 30) {
        for (int row = 1; row < 30; row++) {
            for (int col = 0; col < 75; col++) {
                term_output[row-1][col] = term_output[row][col];
            }
        }
        for (int col = 0; col < 75; col++) {
            term_output[29][col] = '\0';
        }
        term_cursor_row--;
    }
}

void term_io_print(const char* str) {
    while (*str) {
        term_io_putchar(*str++);
    }
    term_io_force_redraw();
}
void term_io_render(int x, int y) {
    // Clear the terminal area first
    fill_rect(x, y, 75 * 8, 30 * 16, 0x000000);
    
    // Draw all characters from buffer
    for (int row = 0; row < 30; row++) {
        for (int col = 0; col < 75; col++) {
            char c = term_output[row][col];
            if (c != '\0') {
                font_draw_char(x + col * 8, y + row * 16, c, 0x00FF00, 0x000000);
            }
        }
    }
    
    // Draw cursor
    font_draw_char(x + term_cursor_col * 8, y + term_cursor_row * 16, '_', 0x00FF00, 0x000000);
}

int term_io_is_active(void) {
    return term_active;  // was hardcoded to 1
}

void term_io_deactivate(void) {
    term_active = 0;
}

void term_io_force_redraw(void) {
    needs_redraw = 1;
}