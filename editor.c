#include "editor.h"
#include "screen.h"
#include "keyboard.h"
#include "fs.h"
#include "cursor.h"

#define MAX_LINES 100
#define MAX_LINE_LEN 80

// Editor state
static char lines[MAX_LINES][MAX_LINE_LEN + 1];
static int line_count;
static int cursor_row;
static int cursor_col;
static char filename[32];
static int dirty;
static int editor_active;

// Helper: string length
static int str_len(const char* s)
{
    int len = 0;
    while (s[len]) len++;
    return len;
}

// Helper: string copy
static void str_cpy(char* dest, const char* src)
{
    while (*src) {
        *dest++ = *src++;
    }
    *dest = '\0';
}

// Helper: clear a line
static void clear_line(char* line)
{
    for (int i = 0; i < MAX_LINE_LEN; i++) {
        line[i] = '\0';
    }
}

// Clear the editor screen area
static void editor_clear_area(void)
{
    for (int row = 0; row < 23; row++) {
        for (int col = 0; col < 80; col++) {
            screen_put_char_at(row, col, ' ', 0x07);
        }
    }
}

// Draw a single line
static void editor_draw_line(int row)
{
    if (row >= line_count) {
        // Clear line if beyond content
        for (int col = 0; col < 80; col++) {
            screen_put_char_at(row, col, ' ', 0x07);
        }
        return;
    }
    
    for (int col = 0; col < 80; col++) {
        if (lines[row][col]) {
            screen_put_char_at(row, col, lines[row][col], 0x07);
        } else {
            screen_put_char_at(row, col, ' ', 0x07);
        }
    }
}

// Draw status bar
static void editor_draw_status(void)
{
    // Status bar background
    for (int col = 0; col < 80; col++) {
        screen_put_char_at(23, col, ' ', 0x1F);
    }
    
    // Filename
    int pos = 0;
    for (int i = 0; filename[i] && i < 20; i++) {
        screen_put_char_at(23, pos++, filename[i], 0x1F);
    }
    
    // Dirty indicator
    if (dirty) {
        screen_put_char_at(23, pos++, ' ', 0x1F);
        screen_put_char_at(23, pos++, '*', 0x1F);
    }
    
    // Position info
    char pos_str[32];
    int pi = 0;
    pos_str[pi++] = ' ';
    pos_str[pi++] = 'L';
    pos_str[pi++] = 'n';
    pos_str[pi++] = ' ';
    
    int line_num = cursor_row + 1;
    if (line_num >= 100) pos_str[pi++] = '0' + (line_num / 100);
    if (line_num >= 10) pos_str[pi++] = '0' + ((line_num % 100) / 10);
    pos_str[pi++] = '0' + (line_num % 10);
    
    pos_str[pi++] = ' ';
    pos_str[pi++] = 'C';
    pos_str[pi++] = 'o';
    pos_str[pi++] = 'l';
    pos_str[pi++] = ' ';
    
    int col_num = cursor_col + 1;
    if (col_num >= 100) pos_str[pi++] = '0' + (col_num / 100);
    if (col_num >= 10) pos_str[pi++] = '0' + ((col_num % 100) / 10);
    pos_str[pi++] = '0' + (col_num % 10);
    
    for (int i = 0; i < pi && pos + i < 80; i++) {
        screen_put_char_at(23, pos + i, pos_str[i], 0x1F);
    }
    
    // Help text
    char help[] = " Ctrl+S Save  Ctrl+Q Quit";
    for (int i = 0; help[i] && pos + pi + i < 80; i++) {
        screen_put_char_at(23, 80 - 23 + i, help[i], 0x1F);
    }
}

// Draw entire editor
static void editor_draw(void)
{
    editor_clear_area();
    
    for (int i = 0; i < line_count && i < 23; i++) {
        editor_draw_line(i);
    }
    
    editor_draw_status();
    update_cursor(cursor_row, cursor_col);
}

// Load file from filesystem
// Load file from filesystem
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
        
        // Handle last line
        if (line_pos > 0 || line_count == 0) {
            lines[line_count][line_pos] = '\0';
            line_count++;
        }
    }
    
    // Ensure at least one line
    if (line_count == 0) {
        line_count = 1;
        lines[0][0] = '\0';
    }
    

    // Set cursor to end of last line
    cursor_row = line_count - 1;
    cursor_col = str_len(lines[line_count - 1]);
}

// Save file to filesystem
static void editor_save(void)
{
    char buffer[16384];
    int pos = 0;
    
    for (int i = 0; i < line_count; i++) {
        for (int j = 0; lines[i][j] && pos < 16383; j++) {
            buffer[pos++] = lines[i][j];
        }
        buffer[pos++] = '\n';
    }
    buffer[pos] = '\0';
    
    if (fs_write(filename, buffer, pos) == 0) {
        dirty = 0;
        print("\nSaved!");
    } else {
        print("\nSave failed!");
    }
    
    // Wait for key press
    keyboard_getchar();
    editor_draw();
}

// Insert character at cursor
static void editor_insert_char(char c)
{
    char* line = lines[cursor_row];
    int len = str_len(line);
    
    if (len >= MAX_LINE_LEN) return;
    
    // Make space for new character
    for (int i = len; i >= cursor_col; i--) {
        line[i + 1] = line[i];
    }
    
    // Insert character
    line[cursor_col] = c;
    cursor_col++;
    dirty = 1;
    
    // Redraw line and update cursor
    editor_draw_line(cursor_row);
    update_cursor(cursor_row, cursor_col);
}

// Delete character before cursor (backspace)
static void editor_backspace(void)
{
    if (cursor_col > 0) {
        char* line = lines[cursor_row];
        int len = str_len(line);
        
        for (int i = cursor_col - 1; i < len; i++) {
            line[i] = line[i + 1];
        }
        cursor_col--;
        dirty = 1;
        editor_draw_line(cursor_row);
        update_cursor(cursor_row, cursor_col);
    } 
    else if (cursor_row > 0) {
        // Merge with previous line
        char* current = lines[cursor_row];
        char* prev = lines[cursor_row - 1];
        int prev_len = str_len(prev);
        
        // Append current to prev
        for (int i = 0; current[i] && prev_len + i < MAX_LINE_LEN; i++) {
            prev[prev_len + i] = current[i];
        }
        prev[prev_len + str_len(current)] = '\0';
        
        // Shift remaining lines up
        for (int i = cursor_row; i < line_count - 1; i++) {
            // Manual line copy
            for (int j = 0; j <= MAX_LINE_LEN; j++) {
                lines[i][j] = lines[i + 1][j];
            }
        }
        line_count--;
        cursor_row--;
        cursor_col = prev_len;
        dirty = 1;
        editor_draw();
    }
}

// Delete character at cursor (Delete key)
static void editor_delete(void)
{
    char* line = lines[cursor_row];
    int len = str_len(line);
    
    if (cursor_col < len) {
        for (int i = cursor_col; i < len; i++) {
            line[i] = line[i + 1];
        }
        dirty = 1;
        editor_draw_line(cursor_row);
        update_cursor(cursor_row, cursor_col);
    }
    else if (cursor_row < line_count - 1) {
        // Merge with next line
        char* current = lines[cursor_row];
        char* next = lines[cursor_row + 1];
        
        // Append next to current
        int cur_len = str_len(current);
        for (int i = 0; next[i] && cur_len + i < MAX_LINE_LEN; i++) {
            current[cur_len + i] = next[i];
        }
        current[cur_len + str_len(next)] = '\0';
        
        // Shift remaining lines up
        for (int i = cursor_row + 1; i < line_count - 1; i++) {
            for (int j = 0; j <= MAX_LINE_LEN; j++) {
                lines[i][j] = lines[i + 1][j];
            }
        }
        line_count--;
        dirty = 1;
        editor_draw();
    }
}

// Insert newline
// Insert newline
static void editor_newline(void)
{
    char* current_line = lines[cursor_row];
    int len = str_len(current_line);
    
    // Create new line with text AFTER cursor
    char new_line[MAX_LINE_LEN + 1];
    int new_len = 0;
    for (int i = cursor_col; i < len; i++) {
        new_line[new_len++] = current_line[i];
    }
    new_line[new_len] = '\0';
    
    // Truncate current line at cursor
    current_line[cursor_col] = '\0';
    
    // Shift all lines down
    for (int i = line_count; i > cursor_row + 1; i--) {
        for (int j = 0; j <= MAX_LINE_LEN; j++) {
            lines[i][j] = lines[i - 1][j];
        }
    }
    
    // Insert the new line
    for (int i = 0; i <= new_len; i++) {
        lines[cursor_row + 1][i] = new_line[i];
    }
    
    line_count++;
    cursor_row++;
    cursor_col = 0;
    dirty = 1;
    editor_draw();
}

// Reset editor state for new file
static void editor_reset(void)
{
    // Clear all lines
    for (int i = 0; i < MAX_LINES; i++) {
        for (int j = 0; j < MAX_LINE_LEN; j++) {
            lines[i][j] = '\0';
        }
    }
    
    line_count = 0;
    cursor_row = 0;
    cursor_col = 0;
    dirty = 0;
    editor_active = 0;
    
    // Clear filename
    for (int i = 0; i < 32; i++) {
        filename[i] = '\0';
    }
}

// Main editor entry point
void editor_open(const char* name)
{
    // Reset state completely
    editor_reset();
    
    // Copy filename
    for (int i = 0; i < 31 && name[i]; i++) {
        filename[i] = name[i];
    }
    filename[31] = '\0';
    
    // Load the file
    editor_load(name);
    dirty = 0;
    editor_active = 1;
    
    // Draw editor
    clear_screen();
    editor_draw();
    
    // Main loop
    int running = 1;
    while (running && editor_active) {
        char c = keyboard_getchar();
        
        if (c == 17) { // Ctrl+Q
            if (dirty) {
                print("\nSave changes? (y/n): ");
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
        else if (c == KEY_UP) {
            if (cursor_row > 0) {
                cursor_row--;
                int line_len = str_len(lines[cursor_row]);
                if (cursor_col > line_len) {
                    cursor_col = line_len;
                }
                update_cursor(cursor_row, cursor_col);
            }
        }
        else if (c == KEY_DOWN) {
            if (cursor_row < line_count - 1) {
                cursor_row++;
                int line_len = str_len(lines[cursor_row]);
                if (cursor_col > line_len) {
                    cursor_col = line_len;
                }
                update_cursor(cursor_row, cursor_col);
            }
        }
        else if (c == KEY_LEFT) {
            if (cursor_col > 0) {
                cursor_col--;
                update_cursor(cursor_row, cursor_col);
            }
        }
        else if (c == KEY_RIGHT) {
            int line_len = str_len(lines[cursor_row]);
            if (cursor_col < line_len) {
                cursor_col++;
                update_cursor(cursor_row, cursor_col);
            }
        }
        else if (c == KEY_DELETE) {
            editor_delete();
        }
        else if (c >= 32 && c <= 126) {
            editor_insert_char(c);
        }
    }
    
    // Clean up and return to shell
    clear_screen();
}