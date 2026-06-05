#include "screen.h"
#include "keyboard.h"
#include "commands.h"
#include "cursor.h"

#define MAX_HISTORY 10
#define MAX_CMD_LEN 256

static char history[MAX_HISTORY][MAX_CMD_LEN];
static int history_count = 0;
static int history_pos = -1;
static char cmdline[MAX_CMD_LEN];
static int cmdlen = 0;

void shell_start(void)
{
    enable_cursor();
    char c;
    
    while (1) {
        // Reset for new command
        cmdline[0] = '\0';
        cmdlen = 0;
        history_pos = -1;
        
        print("> ");
        
        while (1) {
            c = keyboard_getchar();
            
            if (c == '\n') {
                print("\n");
                if (cmdlen > 0) {
                    // Add to history
                    for (int i = MAX_HISTORY - 1; i > 0; i--) {
                        for (int j = 0; j < MAX_CMD_LEN && history[i-1][j]; j++) {
                            history[i][j] = history[i-1][j];
                        }
                    }
                    for (int i = 0; i <= cmdlen; i++) {
                        history[0][i] = cmdline[i];
                    }
                    if (history_count < MAX_HISTORY) history_count++;
                    
                    execute_command(cmdline);
                }
                break;
            }
            else if (c == '\b') {
                if (cmdlen > 0) {
                    cmdlen--;
                    put_char('\b');
                    put_char(' ');
                    put_char('\b');
                    cmdline[cmdlen] = '\0';
                }
            }
            else if (c == KEY_UP) {
                if (history_pos < history_count - 1) {
                    history_pos++;
                    // Clear current line
                    for (int i = 0; i < cmdlen; i++) put_char('\b');
                    for (int i = 0; i < cmdlen; i++) put_char(' ');
                    for (int i = 0; i < cmdlen; i++) put_char('\b');
                    // Display history
                    for (int i = 0; history[history_pos][i]; i++) {
                        put_char(history[history_pos][i]);
                        cmdline[i] = history[history_pos][i];
                    }
                    cmdlen = 0;
                    while (cmdline[cmdlen]) cmdlen++;
                    cmdline[cmdlen] = '\0';
                }
            }
            else if (c == KEY_DOWN) {
                if (history_pos >= 0) {
                    history_pos--;
                    // Clear current line
                    for (int i = 0; i < cmdlen; i++) put_char('\b');
                    for (int i = 0; i < cmdlen; i++) put_char(' ');
                    for (int i = 0; i < cmdlen; i++) put_char('\b');
                    
                    if (history_pos >= 0) {
                        for (int i = 0; history[history_pos][i]; i++) {
                            put_char(history[history_pos][i]);
                            cmdline[i] = history[history_pos][i];
                        }
                        cmdlen = 0;
                        while (cmdline[cmdlen]) cmdlen++;
                        cmdline[cmdlen] = '\0';
                    } else {
                        cmdline[0] = '\0';
                        cmdlen = 0;
                    }
                }
            }
            else if (c == KEY_LEFT) {
                if (cmdlen > 0) {
                    // Move cursor left - just update cursor position
                    // For simplicity, we'll just handle this later
                    put_char('\b');
                }
            }
            else if (c == KEY_RIGHT) {
                // Move cursor right
                // For simplicity, we'll handle later
            }
            else if (c >= ' ' && c <= '~') {
                if (cmdlen < MAX_CMD_LEN - 1) {
                    cmdline[cmdlen++] = c;
                    cmdline[cmdlen] = '\0';
                    put_char(c);
                }
            }
        }
    }
}