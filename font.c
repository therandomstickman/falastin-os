#include "font.h"
#include "graphics.h"

// These symbols are created by objcopy from the binary blob
extern uint8_t _binary_font_psf_start[];
extern uint8_t _binary_font_psf_end[];

// PSF1 header
typedef struct {
    uint8_t magic[2];   // 0x36, 0x04
    uint8_t mode;
    uint8_t charsize;   // bytes per glyph (= height, width is always 8)
} __attribute__((packed)) PSF1Header;

static PSF1Header* psf;
static uint8_t*    glyphs;
static int         char_h;

void font_init(void) {
    psf     = (PSF1Header*)_binary_font_psf_start;
    glyphs  = _binary_font_psf_start + sizeof(PSF1Header);
    char_h  = psf->charsize;
}

void font_draw_char(int x, int y, char c, uint32_t fg, uint32_t bg) {
    uint8_t* glyph = glyphs + (uint8_t)c * char_h;

    for (int row = 0; row < char_h; row++) {
        uint8_t bits = glyph[row];
        for (int col = 0; col < 8; col++) {
            if (bits & (0x80 >> col))
                put_pixel(x + col, y + row, fg);
            else
                put_pixel(x + col, y + row, bg);
        }
    }
}

void font_draw_string(int x, int y, const char* str, uint32_t fg, uint32_t bg) {
    int cx = x;
    while (*str) {
        if (*str == '\n') {
            cx  = x;
            y  += char_h;
        } else {
            font_draw_char(cx, y, *str, fg, bg);
            cx += 8;
        }
        str++;
    }
}

int font_char_width(void)  { return 8; }
int font_char_height(void) { return char_h; }