#include "graphics.h"
#include "font.h"
#include "keyboard.h"
#include "commands.h"
#include "brickwm.h"
#include "term_io.h"
#include "screen.h"
#include "term.h"
#include "net.h"

static char cmdline[256];
static int  cmdlen   = 0;
static int  term_active  = 1;
static int  term_handle  = -1;
extern void display_http_response_if_ready(void);

static int strcmp_term(const char* a, const char* b) {
    while (*a && *b && *a == *b) { a++; b++; }
    return *a - *b;
}

void term_draw(int x, int y, int w, int h) {
    (void)w; (void)h;
    term_io_render(x, y);
}

void term_handle_key(char c) {
    if (!term_active) return;
   

    if (c == '\n') {
        term_io_putchar('\n');
        cmdline[cmdlen] = '\0';

        if (cmdlen > 0) {
            if (strcmp_term(cmdline, "exit") == 0) {
                term_active = 0;
                if (term_handle >= 0)
                    brickwm_close_window(term_handle);
                return;
            }
            // Run the command — output goes to terminal via screen_set_terminal_output
            screen_set_terminal_output(1);
            execute_command(cmdline);
            screen_set_terminal_output(0);
        }

        cmdlen = 0;
        cmdline[0] = '\0';
        term_io_print("> ");

    } else if (c == '\b') {
        if (cmdlen > 0) {
            cmdlen--;
            cmdline[cmdlen] = '\0';
            term_io_putchar('\b');
        }
    } else if (c >= ' ' && c <= '~') {
        if (cmdlen < 255) {
            cmdline[cmdlen++] = c;
            cmdline[cmdlen]   = '\0';
            term_io_putchar(c);
        }
    }
}

void term_set_window_handle(int handle) {
    term_handle = handle;
}

void terminal_run(void) {
    // Remove the term_started guard entirely, reset state instead
    term_io_init();
    term_active = 1;
    cmdlen = 0;
    cmdline[0] = '\0';
    term_handle = -1;

    term_io_print("FalastinOS Terminal\n");
    term_io_print("Type 'exit' to close\n\n> ");

    term_handle = brickwm_create_window(
        "Terminal", 150, 100, 500, 350,
        term_draw, term_handle_key
    );
    term_set_window_handle(term_handle);
}