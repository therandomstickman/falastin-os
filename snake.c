#include "gamemode.h"
#include "screen.h"
#include "timer.h"
#include "libc.h"
#include "keyboard.h"
#include "graphics.h"

// Simple random number generator
static unsigned int rng_state = 1;

static int my_rand(void) {
    rng_state = rng_state * 1103515245 + 12345;
    return (unsigned int)(rng_state / 65536) % 32768;
}

// Game constants (use current framebuffer resolution)
static int width, height;
static int cell_size = 16;
static int grid_w, grid_h;

// Snake parts
typedef struct {
    int x, y;
} Point;

static Point snake[100];
static int snake_len = 3;
static int dir_x = 1, dir_y = 0;
static int food_x, food_y;
static int score = 0;
static int game_over = 0;

// Colors (RGB)
#define COLOR_BLACK   0x000000
#define COLOR_GREEN   0x00FF00
#define COLOR_RED     0xFF0000
#define COLOR_YELLOW  0xFFFF00
#define COLOR_WHITE   0xFFFFFF

// Forward declarations
static void draw_game(void);
static void generate_food(void);

static void draw_rect(int x, int y, int w, int h, uint32_t color) {
    fill_rect(x, y, w, h, color);
}

static void draw_snake_cell(int x, int y, int color) {
    draw_rect(x * cell_size, y * cell_size, cell_size - 1, cell_size - 1, color);
}

static void draw_number(int x, int y, int num) {
    char str[16];
    itoa(num, str, 10);
    for (int i = 0; str[i]; i++) {
        for (int py = 0; py < 8; py++) {
            for (int px = 0; px < 6; px++) {
                put_pixel(x + i * 8 + px, y + py, COLOR_WHITE);
            }
        }
    }
}

static void generate_food(void) {
    int attempts = 0;
    while (attempts < 1000) {
        int fx = my_rand() % grid_w;
        int fy = my_rand() % grid_h;
        int collision = 0;
        
        for (int i = 0; i < snake_len; i++) {
            if (snake[i].x == fx && snake[i].y == fy) {
                collision = 1;
                break;
            }
        }
        
        if (!collision) {
            food_x = fx;
            food_y = fy;
            return;
        }
        attempts++;
    }
    
    // Fallback: find any empty spot
    for (int y = 0; y < grid_h; y++) {
        for (int x = 0; x < grid_w; x++) {
            int collision = 0;
            for (int i = 0; i < snake_len; i++) {
                if (snake[i].x == x && snake[i].y == y) {
                    collision = 1;
                    break;
                }
            }
            if (!collision) {
                food_x = x;
                food_y = y;
                return;
            }
        }
    }
}

static void init_game(void) {
    // Get screen dimensions
    width = fb_width_get();
    height = fb_height_get();
    cell_size = (width < height) ? width / 40 : height / 30;
    if (cell_size < 8) cell_size = 8;
    if (cell_size > 20) cell_size = 20;
    
    grid_w = width / cell_size;
    grid_h = height / cell_size;
    
    // Initialize random seed
    extern uint32_t timer_ticks(void);
    rng_state = timer_ticks();
    
    // Start snake in the middle
    int center_x = grid_w / 2;
    int center_y = grid_h / 2;
    
    snake[0].x = center_x;
    snake[0].y = center_y;
    snake[1].x = center_x - 1;
    snake[1].y = center_y;
    snake[2].x = center_x - 2;
    snake[2].y = center_y;
    snake_len = 3;
    dir_x = 1;
    dir_y = 0;
    score = 0;
    game_over = 0;
    
    generate_food();
    
    // Clear screen
    fill_screen(COLOR_BLACK);
}

static void draw_game(void) {
    // Draw snake
    for (int i = 0; i < snake_len; i++) {
        draw_snake_cell(snake[i].x, snake[i].y, COLOR_GREEN);
    }
    
    // Draw head
    if (!game_over && snake_len > 0) {
        draw_snake_cell(snake[0].x, snake[0].y, COLOR_YELLOW);
    }
    
    // Draw food
    draw_snake_cell(food_x, food_y, COLOR_RED);
    
    // Draw score
    draw_number(10, height - 30, score);
    
    // Game over message
    if (game_over) {
        const char* msg = "GAME OVER! Press ESC";
        int msg_w = 0;
        while (msg[msg_w]) msg_w++;
        int start_x = (width - msg_w * 8) / 2;
        int start_y = height / 2;
        for (int i = 0; msg[i]; i++) {
            for (int py = 0; py < 8; py++) {
                for (int px = 0; px < 6; px++) {
                    put_pixel(start_x + i * 8 + px, start_y + py, COLOR_RED);
                }
            }
        }
    }
}

static void move_snake(void) {
    if (game_over) return;
    
    int new_x = snake[0].x + dir_x;
    int new_y = snake[0].y + dir_y;
    
    // Wall collision
    if (new_x < 0 || new_x >= grid_w || new_y < 0 || new_y >= grid_h) {
        game_over = 1;
        return;
    }
    
    int ate_food = (new_x == food_x && new_y == food_y);
    
    if (ate_food) {
        snake_len++;
        score++;
        for (int i = snake_len - 1; i > 0; i--) {
            snake[i] = snake[i-1];
        }
        snake[0].x = new_x;
        snake[0].y = new_y;
        generate_food();
    } else {
        for (int i = snake_len - 1; i > 0; i--) {
            snake[i] = snake[i-1];
        }
        snake[0].x = new_x;
        snake[0].y = new_y;
    }
    
    // Self collision
    for (int i = 1; i < snake_len; i++) {
        if (snake[0].x == snake[i].x && snake[0].y == snake[i].y) {
            game_over = 1;
            return;
        }
    }
}

static void handle_input(void) {
    if (keyboard_has_char()) {
        int scancode = keyboard_getchar();
        
        switch (scancode) {
            case 0x01: // ESC
                gamemode_exit();
                break;
            case 0x48: // Up
                if (dir_y == 0) { dir_x = 0; dir_y = -1; }
                break;
            case 0x50: // Down
                if (dir_y == 0) { dir_x = 0; dir_y = 1; }
                break;
            case 0x4B: // Left
                if (dir_x == 0) { dir_x = -1; dir_y = 0; }
                break;
            case 0x4D: // Right
                if (dir_x == 0) { dir_x = 1; dir_y = 0; }
                break;
        }
    }
}

static void snake_init(void) {
    init_game();
    draw_game(); // Draw initial state
}

static void snake_run(void) {
    int frame_delay = 0;
    
    while (gamemode_active()) {
        handle_input();
        
        frame_delay++;
        if (frame_delay < 8) continue;
        frame_delay = 0;
        
        move_snake();
        draw_game();
    }
}

static void snake_cleanup(void) {
    fill_screen(0x00003366); // Restore desktop color
}

static Game snake_game = {
    .name = "Snake",
    .init = snake_init,
    .run = snake_run,
    .cleanup = snake_cleanup
};

void snake_start(void) {
    gamemode_enter(&snake_game);
}