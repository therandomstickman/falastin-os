#include "editor.h"
#include "screen.h"
#include "keyboard.h"
#include "cursor.h"
#include "fs.h"
#include "libc.h"

#define EDIT_ROWS 22
#define EDIT_COLS 80
#define MAX_LINES 128
#define MAX_LINE_LEN 120
#define FILE_BUFFER_SIZE 16384

static char lines[MAX_LINES][MAX_LINE_LEN + 1];
static int line_count;
static int cursor_row;
static int cursor_col;
static int top_line;
static int dirty;
static char filename[64];

static int line_len(int row) {
    if (row < 0 || row >= line_count) return 0;
    return (int)strlen(lines[row]);
}

static void copy_string(char* dest, const char* src, int max_len) {
    int i = 0;
    while (src[i] && i < max_len - 1) {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

static void clear_editor_state(void) {
    for (int row = 0; row < MAX_LINES; row++) {
        lines[row][0] = '\0';
    }
    line_count = 1;
    cursor_row = 0;
    cursor_col = 0;
    top_line = 0;
    dirty = 0;
}

static void clear_screen_line(int row, char color) {
    for (int col = 0; col < EDIT_COLS; col++) {
        screen_put_char_at(row, col, ' ', color);
    }
}

static void draw_string_at(int row, int col, const char* text, char color) {
    int i = 0;
    while (text[i] && col + i < EDIT_COLS) {
        screen_put_char_at(row, col + i, text[i], color);
        i++;
    }
}

static void draw_number_at(int row, int col, int value, char color) {
    char buf[16];
    itoa(value, buf, 10);
    draw_string_at(row, col, buf, color);
}

static void ensure_cursor_visible(void) {
    if (cursor_row < top_line) {
        top_line = cursor_row;
    }
    if (cursor_row >= top_line + EDIT_ROWS) {
        top_line = cursor_row - EDIT_ROWS + 1;
    }
    if (top_line < 0) top_line = 0;
}

static void draw_status(void) {
    clear_screen_line(EDIT_ROWS, 0x1F);
    draw_string_at(EDIT_ROWS, 0, "EDIT ", 0x1F);
    draw_string_at(EDIT_ROWS, 5, filename, 0x1F);
    if (dirty) {
        draw_string_at(EDIT_ROWS, 5 + (int)strlen(filename), " *", 0x1F);
    }

    draw_string_at(EDIT_ROWS, 32, "Ln ", 0x1F);
    draw_number_at(EDIT_ROWS, 35, cursor_row + 1, 0x1F);
    draw_string_at(EDIT_ROWS, 42, "Col ", 0x1F);
    draw_number_at(EDIT_ROWS, 46, cursor_col + 1, 0x1F);
    draw_string_at(EDIT_ROWS, 58, "Ctrl+S save Ctrl+Q quit", 0x1F);
}

static void draw_message(const char* msg) {
    clear_screen_line(EDIT_ROWS + 1, 0x07);
    draw_string_at(EDIT_ROWS + 1, 0, msg, 0x07);
}

static void draw_editor(void) {
    ensure_cursor_visible();

    for (int screen_row = 0; screen_row < EDIT_ROWS; screen_row++) {
        int file_row = top_line + screen_row;
        clear_screen_line(screen_row, 0x07);

        if (file_row < line_count) {
            for (int col = 0; col < EDIT_COLS && lines[file_row][col]; col++) {
                screen_put_char_at(screen_row, col, lines[file_row][col], 0x07);
            }
        }
    }

    draw_status();
    update_cursor(cursor_row - top_line, cursor_col);
}

static void load_file(const char* name) {
    char buffer[FILE_BUFFER_SIZE];
    int bytes = fs_read(name, buffer, FILE_BUFFER_SIZE - 1);

    clear_editor_state();

    if (bytes <= 0) {
        return;
    }

    buffer[bytes] = '\0';
    line_count = 1;
    int col = 0;

    for (int i = 0; i < bytes && line_count <= MAX_LINES; i++) {
        char c = buffer[i];
        if (c == '\r') {
            continue;
        }
        if (c == '\n') {
            lines[line_count - 1][col] = '\0';
            if (line_count < MAX_LINES) {
                line_count++;
                col = 0;
                lines[line_count - 1][0] = '\0';
            }
            continue;
        }
        if (col < MAX_LINE_LEN) {
            lines[line_count - 1][col++] = c;
            lines[line_count - 1][col] = '\0';
        }
    }

    cursor_row = 0;
    cursor_col = 0;
    top_line = 0;
    dirty = 0;
}

static int save_file(void) {
    char buffer[FILE_BUFFER_SIZE];
    int pos = 0;

    for (int row = 0; row < line_count && pos < FILE_BUFFER_SIZE - 1; row++) {
        for (int col = 0; lines[row][col] && pos < FILE_BUFFER_SIZE - 1; col++) {
            buffer[pos++] = lines[row][col];
        }
        if (pos < FILE_BUFFER_SIZE - 1) {
            buffer[pos++] = '\n';
        }
    }

    if (fs_write(filename, buffer, pos) == 0) {
        dirty = 0;
        draw_message("Saved.");
        return 1;
    }

    draw_message("Save failed.");
    return 0;
}

static void insert_char(char c) {
    int len = line_len(cursor_row);
    if (len >= MAX_LINE_LEN) return;

    for (int i = len; i >= cursor_col; i--) {
        lines[cursor_row][i + 1] = lines[cursor_row][i];
    }
    lines[cursor_row][cursor_col] = c;
    cursor_col++;
    dirty = 1;
}

static void insert_newline(void) {
    if (line_count >= MAX_LINES) return;

    int len = line_len(cursor_row);
    for (int row = line_count; row > cursor_row + 1; row--) {
        memcpy(lines[row], lines[row - 1], MAX_LINE_LEN + 1);
    }

    copy_string(lines[cursor_row + 1], lines[cursor_row] + cursor_col, MAX_LINE_LEN + 1);
    lines[cursor_row][cursor_col] = '\0';
    line_count++;
    cursor_row++;
    cursor_col = 0;
    (void)len;
    dirty = 1;
}

static void backspace_char(void) {
    if (cursor_col > 0) {
        int len = line_len(cursor_row);
        for (int i = cursor_col - 1; i < len; i++) {
            lines[cursor_row][i] = lines[cursor_row][i + 1];
        }
        cursor_col--;
        dirty = 1;
        return;
    }

    if (cursor_row > 0) {
        int prev_len = line_len(cursor_row - 1);
        int cur_len = line_len(cursor_row);
        int copy_len = MAX_LINE_LEN - prev_len;
        if (copy_len > cur_len) copy_len = cur_len;

        for (int i = 0; i < copy_len; i++) {
            lines[cursor_row - 1][prev_len + i] = lines[cursor_row][i];
        }
        lines[cursor_row - 1][prev_len + copy_len] = '\0';

        for (int row = cursor_row; row < line_count - 1; row++) {
            memcpy(lines[row], lines[row + 1], MAX_LINE_LEN + 1);
        }
        line_count--;
        cursor_row--;
        cursor_col = prev_len;
        dirty = 1;
    }
}

static void delete_char(void) {
    int len = line_len(cursor_row);

    if (cursor_col < len) {
        for (int i = cursor_col; i < len; i++) {
            lines[cursor_row][i] = lines[cursor_row][i + 1];
        }
        dirty = 1;
        return;
    }

    if (cursor_row < line_count - 1) {
        cursor_row++;
        cursor_col = 0;
        backspace_char();
    }
}

static void move_cursor(int drow, int dcol) {
    cursor_row += drow;
    if (cursor_row < 0) cursor_row = 0;
    if (cursor_row >= line_count) cursor_row = line_count - 1;

    cursor_col += dcol;
    if (cursor_col < 0) cursor_col = 0;

    int len = line_len(cursor_row);
    if (cursor_col > len) cursor_col = len;
}

void editor_open(const char* name) {
    copy_string(filename, name, sizeof(filename));
    load_file(filename);

    clear_screen();
    draw_message("Editing. Ctrl+S saves, Ctrl+Q quits.");
    draw_editor();

    int running = 1;
    while (running) {
        char c = keyboard_getchar();

        if (c == 17) {
            running = 0;
        } else if (c == 19) {
            save_file();
        } else if (c == '\n') {
            insert_newline();
        } else if (c == '\b') {
            backspace_char();
        } else if (c == KEY_DELETE) {
            delete_char();
        } else if (c == KEY_UP) {
            move_cursor(-1, 0);
        } else if (c == KEY_DOWN) {
            move_cursor(1, 0);
        } else if (c == KEY_LEFT) {
            move_cursor(0, -1);
        } else if (c == KEY_RIGHT) {
            move_cursor(0, 1);
        } else if (c >= 32 && c <= 126) {
            insert_char(c);
        }

        draw_editor();
    }

    clear_screen();
    if (dirty) {
        print("Unsaved changes discarded.\n");
    }
}
