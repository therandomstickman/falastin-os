#include "doom_compat.h"
#include "graphics.h"
#include "screen.h"
#include "libc.h"
#include "font.h"
#include "keyboard.h"

#define MAP_W 16
#define MAP_H 16
#define FP_SHIFT 8
#define FP_ONE (1 << FP_SHIFT)
#define FOV 60
#define MOVE_STEP 56
#define TURN_STEP 5
#define MAX_RAY_STEPS 220
#define RENDER_STEP 2
#define MAX_FB_W 800
#define MAX_FB_H 600

static const char doom_map[MAP_H][MAP_W + 1] = {
    "1111111111111111",
    "1000000000000001",
    "1011110111110101",
    "1010000100010101",
    "1010111101010101",
    "1000100001010001",
    "1110101111011101",
    "1000101000010001",
    "1011101011110101",
    "1000001000000101",
    "1011111110111101",
    "1010000000100001",
    "1010111111101101",
    "1000100000000001",
    "1000000111000001",
    "1111111111111111"
};

static uint32_t doom_palette[256];
static uint32_t frame_buffer[MAX_FB_W * MAX_FB_H];
static uint32_t* draw_buffer = 0;
static int draw_w = 0;
static int draw_h = 0;
static int draw_uses_backbuffer = 0;
static int player_x = 1 * FP_ONE + FP_ONE / 2;
static int player_y = 1 * FP_ONE + FP_ONE / 2;
static int player_angle = 0;

static int abs_int(int v) {
    return v < 0 ? -v : v;
}

static int clamp_int(int v, int lo, int hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static int wrap_angle(int angle) {
    while (angle < 0) angle += 360;
    while (angle >= 360) angle -= 360;
    return angle;
}

static int sin_fixed(int angle) {
    angle = wrap_angle(angle);
    int sign = 1;

    if (angle >= 180) {
        angle -= 180;
        sign = -1;
    }
    if (angle > 90) {
        angle = 180 - angle;
    }

    // Bhaskara-style approximation scaled to FP_ONE.
    // Gives exact zero at 0 degrees and exact one at 90 degrees.
    int numerator = 4 * angle * (180 - angle) * FP_ONE;
    int denominator = 40500 - angle * (180 - angle);
    int y = denominator ? numerator / denominator : 0;
    return sign * y;
}

static int cos_fixed(int angle) {
    return sin_fixed(angle + 90);
}

static int map_wall_at(int x, int y) {
    int mx = x >> FP_SHIFT;
    int my = y >> FP_SHIFT;

    if (mx < 0 || mx >= MAP_W || my < 0 || my >= MAP_H) return 1;
    return doom_map[my][mx] == '1';
}

static uint32_t shade_color(uint32_t color, int dist) {
    int shade = 255 - dist / 3;
    shade = clamp_int(shade, 48, 255);

    uint32_t r = ((color >> 16) & 0xFF) * shade / 255;
    uint32_t g = ((color >> 8) & 0xFF) * shade / 255;
    uint32_t b = (color & 0xFF) * shade / 255;
    return (r << 16) | (g << 8) | b;
}

static void draw_text(int x, int y, const char* text, uint32_t fg) {
    font_draw_string(x, y, text, fg, 0x000000);
}

static void frame_begin(uint32_t* fb, int fb_w, int fb_h) {
    draw_w = fb_w;
    draw_h = fb_h;
    draw_uses_backbuffer = fb_w <= MAX_FB_W && fb_h <= MAX_FB_H;
    draw_buffer = draw_uses_backbuffer ? frame_buffer : fb;
}

static void frame_present(uint32_t* fb) {
    if (draw_uses_backbuffer) {
        memcpy(fb, draw_buffer, draw_w * draw_h * sizeof(uint32_t));
    }
}

static void buf_put_pixel(int x, int y, uint32_t color) {
    if (x >= 0 && x < draw_w && y >= 0 && y < draw_h) {
        draw_buffer[y * draw_w + x] = color;
    }
}

static void buf_fill_rect(int x, int y, int w, int h, uint32_t color) {
    int x0 = clamp_int(x, 0, draw_w);
    int y0 = clamp_int(y, 0, draw_h);
    int x1 = clamp_int(x + w, 0, draw_w);
    int y1 = clamp_int(y + h, 0, draw_h);

    for (int py = y0; py < y1; py++) {
        uint32_t* row = draw_buffer + py * draw_w;
        for (int px = x0; px < x1; px++) {
            row[px] = color;
        }
    }
}

void doom_init_palette(void) {
    for (int i = 0; i < 256; i++) {
        uint8_t r = (i * 2) & 0xFF;
        uint8_t g = (i * 3) & 0xFF;
        uint8_t b = (i * 4) & 0xFF;
        doom_palette[i] = (r << 16) | (g << 8) | b;
    }
}

void doom_draw_8bit_buffer(uint8_t* buffer, int width, int height) {
    uint32_t* fb = fb_get();
    int fb_w = fb_width_get();
    int fb_h = fb_height_get();

    if (!fb) return;

    for (int y = 0; y < height && y < fb_h; y++) {
        for (int x = 0; x < width && x < fb_w; x++) {
            uint8_t pal_idx = buffer[y * width + x];
            fb[y * fb_w + x] = doom_palette[pal_idx];
        }
    }
}

static void draw_weapon(int fb_w, int fb_h) {
    int base_y = fb_h - 92;
    int cx = fb_w / 2;

    buf_fill_rect(cx - 36, base_y + 28, 72, 64, 0x2d2d2d);
    buf_fill_rect(cx - 22, base_y + 4, 44, 52, 0x555555);
    buf_fill_rect(cx - 12, base_y, 24, 62, 0x888888);
    buf_fill_rect(cx - 5, base_y - 10, 10, 40, 0xb8b8b8);
    buf_fill_rect(cx - 34, base_y + 34, 68, 8, 0x111111);
}

static void draw_status_bar(int fb_w, int fb_h, int health, int ammo) {
    int bar_h = 34;
    int y = fb_h - bar_h;
    (void)health;
    (void)ammo;

    buf_fill_rect(0, y, fb_w, bar_h, 0x3c3c3c);
    buf_fill_rect(0, y, fb_w, 2, 0xb30000);
}

static void draw_minimap(int ox, int oy) {
    int scale = 4;

    for (int y = 0; y < MAP_H; y++) {
        for (int x = 0; x < MAP_W; x++) {
            uint32_t color = doom_map[y][x] == '1' ? 0x777777 : 0x161616;
            buf_fill_rect(ox + x * scale, oy + y * scale, scale - 1, scale - 1, color);
        }
    }

    buf_fill_rect(ox + (player_x >> FP_SHIFT) * scale - 1,
                  oy + (player_y >> FP_SHIFT) * scale - 1,
                  3, 3, 0xff2020);
}

static void render_scene(void) {
    uint32_t* fb = fb_get();
    int fb_w = fb_width_get();
    int fb_h = fb_height_get();

    if (!fb) {
        return;
    }

    int game_h = fb_h - 34;
    frame_begin(fb, fb_w, fb_h);
    buf_fill_rect(0, 0, fb_w, game_h / 2, 0x303850);
    buf_fill_rect(0, game_h / 2, fb_w, game_h / 2, 0x2b241d);

    for (int col = 0; col < fb_w; col += RENDER_STEP) {
        int ray_angle = wrap_angle(player_angle - FOV / 2 + (col * FOV) / fb_w);
        int dx = cos_fixed(ray_angle);
        int dy = sin_fixed(ray_angle);
        int ray_x = player_x;
        int ray_y = player_y;
        int dist = 1;
        int hit_side = 0;

        for (int step = 0; step < MAX_RAY_STEPS; step++) {
            int old_mx = ray_x >> FP_SHIFT;
            int old_my = ray_y >> FP_SHIFT;
            ray_x += dx / 8;
            ray_y += dy / 8;
            dist += 4;

            if (map_wall_at(ray_x, ray_y)) {
                int mx = ray_x >> FP_SHIFT;
                int my = ray_y >> FP_SHIFT;
                hit_side = (mx != old_mx) ? 1 : (my != old_my ? 2 : 0);
                break;
            }
        }

        int corrected = (dist * cos_fixed(ray_angle - player_angle)) >> FP_SHIFT;
        if (corrected < 1) corrected = 1;

        int wall_h = (game_h * 72) / corrected;
        if (wall_h > game_h) wall_h = game_h;
        int top = (game_h - wall_h) / 2;
        int bottom = top + wall_h;
        uint32_t wall = hit_side == 1 ? 0x9a1616 : 0x751010;

        wall = shade_color(wall, corrected);
        buf_fill_rect(col, top, RENDER_STEP, wall_h, wall);

        if (top > 0) {
            uint32_t ceil = shade_color(0x303850, abs_int(game_h / 2 - top));
            buf_fill_rect(col, top, RENDER_STEP, 1, ceil);
        }
        if (bottom < game_h) {
            uint32_t floor = shade_color(0x3a2f23, abs_int(bottom - game_h / 2));
            buf_fill_rect(col, bottom, RENDER_STEP, 1, floor);
        }
    }

    draw_weapon(fb_w, fb_h);
    draw_status_bar(fb_w, fb_h, 100, 50);
    draw_minimap(8, 24);
    frame_present(fb);
    draw_text(10, fb_h - 25, "DOOM", 0xff3030);
    draw_text(82, fb_h - 25, "HEALTH 100", 0xffffff);
    draw_text(210, fb_h - 25, "AMMO 50", 0xffffff);
    draw_text(fb_w - 150, fb_h - 25, "ESC exits", 0xd0d0d0);
    draw_text(8, 8, "WASD / arrows move", 0xffffff);
}

static void try_move(int forward, int strafe) {
    int move_angle = player_angle;
    int amount = forward;

    if (strafe != 0) {
        move_angle = wrap_angle(player_angle + (strafe > 0 ? 90 : -90));
        amount = abs_int(strafe);
    }

    int nx = player_x + ((cos_fixed(move_angle) * amount) >> FP_SHIFT);
    int ny = player_y + ((sin_fixed(move_angle) * amount) >> FP_SHIFT);

    if (!map_wall_at(nx, player_y)) player_x = nx;
    if (!map_wall_at(player_x, ny)) player_y = ny;
}

static int handle_key(int key) {
    switch (key) {
        case 27:
            return 0;
        case KEY_LEFT:
        case 'a':
        case 'A':
            player_angle = wrap_angle(player_angle - TURN_STEP);
            break;
        case KEY_RIGHT:
        case 'd':
        case 'D':
            player_angle = wrap_angle(player_angle + TURN_STEP);
            break;
        case KEY_UP:
        case 'w':
        case 'W':
            try_move(MOVE_STEP, 0);
            break;
        case KEY_DOWN:
        case 's':
        case 'S':
            try_move(-MOVE_STEP, 0);
            break;
        case 'q':
        case 'Q':
            try_move(0, -MOVE_STEP);
            break;
        case 'e':
        case 'E':
            try_move(0, MOVE_STEP);
            break;
    }
    return 1;
}

int doom_main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    print("Starting FalastinOS Doom port...\n");
    doom_init_palette();
    doom_video_init();
    render_scene();

    while (1) {
        if (keyboard_has_char()) {
            int key = doom_get_key();
            if (!handle_key(key)) {
                break;
            }
            render_scene();
        } else {
            doom_sleep(10);
        }
    }

    fill_screen(0x000000);
    print("DOOM exited.\n");
    return 0;
}
