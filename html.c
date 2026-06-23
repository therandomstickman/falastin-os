#include "screen.h"
#include "libc.h"

static void strip_tags(const char* html, char* output, int max_len) {
    int in_tag = 0;
    int out_pos = 0;
    
    for (int i = 0; html[i] && out_pos < max_len - 1; i++) {
        if (html[i] == '<') {
            in_tag = 1;
            continue;
        }
        if (html[i] == '>') {
            in_tag = 0;
            continue;
        }
        if (!in_tag) {
            output[out_pos++] = html[i];
        }
    }
    output[out_pos] = '\0';
}

void render_html(const char* html) {
    char text[4096];
    strip_tags(html, text, sizeof(text));
    
    // Print text with word wrapping
    int col = 0;
    for (int i = 0; text[i]; i++) {
        if (text[i] == '\n' || text[i] == '\r') {
            print("\n");
            col = 0;
            continue;
        }
        
        put_char(text[i]);
        col++;
        if (col >= 80) {
            print("\n");
            col = 0;
        }
    }
    print("\n");
}