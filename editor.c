#include "editor.h"
#include "screen.h"
#include "keyboard.h"
#include "fs.h"
#include "cursor.h"

// Custom strlen function
static int my_strlen(const char* s)
{
    int len = 0;
    while (s[len]) len++;
    return len;
}

#define MAX_LINES 100
#define MAX_LINE_LEN 80

static char lines[MAX_LINES][MAX_LINE_LEN + 1];
static int line_count;
static int cursor_row, cursor_col;
static char filename[32];
static int dirty;

static void editor_draw(void)
{
    clear_screen();
    
    // Draw all lines
    for (int i = 0; i < line_count && i < 23; i++) {
        for (int j = 0; lines[i][j]; j++) {
            screen_put_char_at(i, j, lines[i][j], 0x07);
        }
    }
    
    // Draw status bar
    for (int i = 0; i < 80; i++) {
        screen_put_char_at(23, i, ' ', 0x1F);
    }
    
    int pos = 0;
    for (int i = 0; filename[i] && i < 20; i++) {
        screen_put_char_at(23, pos++, filename[i], 0x1F);
    }
    if (dirty) {
        screen_put_char_at(23, pos++, ' ', 0x1F);
        screen_put_char_at(23, pos++, '*', 0x1F);
    }
    
    screen_put_char_at(23, 50, 'C', 0x1F);
    screen_put_char_at(23, 51, 't', 0x1F);
    screen_put_char_at(23, 52, 'r', 0x1F);
    screen_put_char_at(23, 53, 'l', 0x1F);
    screen_put_char_at(23, 54, '+', 0x1F);
    screen_put_char_at(23, 55, 'S', 0x1F);
    screen_put_char_at(23, 56, ':', 0x1F);
    screen_put_char_at(23, 57, 'S', 0x1F);
    screen_put_char_at(23, 58, 'a', 0x1F);
    screen_put_char_at(23, 59, 'v', 0x1F);
    screen_put_char_at(23, 60, 'e', 0x1F);
    
    update_cursor(cursor_row, cursor_col);
}

static void editor_load(const char* name)
{
    char buffer[16384];
    int bytes = fs_read(name, buffer, sizeof(buffer) - 1);
    
    if (bytes > 0) {
        buffer[bytes] = '\0';
        line_count = 0;
        int line_pos = 0;
        
        for (int i = 0; i < bytes && line_count < MAX_LINES; i++) {
            if (buffer[i] == '\n') {
                lines[line_count][line_pos] = '\0';
                line_count++;
                line_pos = 0;
            } else if (buffer[i] != '\r') {
                if (line_pos < MAX_LINE_LEN) {
                    lines[line_count][line_pos++] = buffer[i];
                }
            }
        }
        if (line_pos > 0 || line_count == 0) {
            lines[line_count][line_pos] = '\0';
            line_count++;
        }
    } else {
        line_count = 1;
        lines[0][0] = '\0';
    }
}

static void editor_save(void)
{
    char buffer[16384];
    int pos = 0;
    
    for (int i = 0; i < line_count; i++) {
        for (int j = 0; lines[i][j]; j++) {
            buffer[pos++] = lines[i][j];
        }
        buffer[pos++] = '\n';
    }
    buffer[pos] = '\0';
    
    if (fs_write(filename, buffer, pos) == 0) {
        dirty = 0;
        print("\nSaved!");
        keyboard_getchar();
    } else {
        print("\nSave failed!");
        keyboard_getchar();
    }
    editor_draw();
}

static void editor_insert_char(char c)
{
    char* line = lines[cursor_row];
    int len = 0;
    while (line[len]) len++;
    
    if (len >= MAX_LINE_LEN) return;
    
    for (int i = len; i >= cursor_col; i--) {
        line[i + 1] = line[i];
    }
    line[cursor_col] = c;
    cursor_col++;
    dirty = 1;
    editor_draw();
}

static void editor_backspace(void)
{
    if (cursor_col > 0) {
        char* line = lines[cursor_row];
        int len = 0;
        while (line[len]) len++;
        for (int i = cursor_col - 1; i < len; i++) {
            line[i] = line[i + 1];
        }
        cursor_col--;
        dirty = 1;
        editor_draw();
    } else if (cursor_row > 0) {
        // Merge with previous line
        char* current = lines[cursor_row];
        char* prev = lines[cursor_row - 1];
        int prev_len = 0;
        while (prev[prev_len]) prev_len++;
        
        // Append current to prev
        for (int i = 0; current[i] && prev_len + i < MAX_LINE_LEN; i++) {
            prev[prev_len + i] = current[i];
        }
        prev[prev_len + my_strlen(current)] = '\0';
        
        // Shift lines up
        for (int i = cursor_row; i < line_count - 1; i++) {
            for (int j = 0; lines[i + 1][j]; j++) {
                lines[i][j] = lines[i + 1][j];
            }
            lines[i][my_strlen(lines[i + 1])] = '\0';
        }
        line_count--;
        cursor_row--;
        cursor_col = prev_len;
        dirty = 1;
        editor_draw();
    }
}

static void editor_newline(void)
{
    char* line = lines[cursor_row];
    int len = 0;
    while (line[len]) len++;
    
    // Create new line with remaining text
    char newline[MAX_LINE_LEN + 1];
    int newlen = 0;
    for (int i = cursor_col; i < len; i++) {
        newline[newlen++] = line[i];
    }
    newline[newlen] = '\0';
    
    // Truncate current line
    line[cursor_col] = '\0';
    
    // Insert new line
    for (int i = line_count; i > cursor_row + 1; i--) {
        for (int j = 0; lines[i - 1][j]; j++) {
            lines[i][j] = lines[i - 1][j];
        }
        lines[i][my_strlen(lines[i - 1])] = '\0';
    }
    
    for (int i = 0; i <= newlen; i++) {
        lines[cursor_row + 1][i] = newline[i];
    }
    line_count++;
    cursor_row++;
    cursor_col = 0;
    dirty = 1;
    editor_draw();
}

void editor_open(const char* name)
{
    for (int i = 0; i < 31 && name[i]; i++) {
        filename[i] = name[i];
    }
    filename[31] = '\0';
    
    editor_load(name);
    cursor_row = 0;
    cursor_col = 0;
    dirty = 0;
    
    editor_draw();
    
    int running = 1;
    while (running) {
        char c = keyboard_getchar();
        
        if (c == 17) { // Ctrl+Q
            if (dirty) {
                print("\nSave? (y/n): ");
                char resp = keyboard_getchar();
                if (resp == 'y' || resp == 'Y') {
                    editor_save();
                }
            }
            running = 0;
        }
        else if (c == 19) { // Ctrl+S
            editor_save();
        }
        else if (c == '\n') {
            editor_newline();
        }
        else if (c == '\b') {
            editor_backspace();
        }
        else if (c == KEY_UP && cursor_row > 0) {
            cursor_row--;
            int len = 0;
            while (lines[cursor_row][len]) len++;
            if (cursor_col > len) cursor_col = len;
            update_cursor(cursor_row, cursor_col);
        }
        else if (c == KEY_DOWN && cursor_row < line_count - 1) {
            cursor_row++;
            int len = 0;
            while (lines[cursor_row][len]) len++;
            if (cursor_col > len) cursor_col = len;
            update_cursor(cursor_row, cursor_col);
        }
        else if (c == KEY_LEFT && cursor_col > 0) {
            cursor_col--;
            update_cursor(cursor_row, cursor_col);
        }
        else if (c == KEY_RIGHT) {
            int len = 0;
            while (lines[cursor_row][len]) len++;
            if (cursor_col < len) {
                cursor_col++;
                update_cursor(cursor_row, cursor_col);
            }
        }
        else if (c >= 32 && c <= 126) {
            editor_insert_char(c);
        }
    }
    
    clear_screen();
}