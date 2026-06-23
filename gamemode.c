#include "gamemode.h"
#include "screen.h"
#include "timer.h"
#include "malloc.h"
#include "libc.h"
#include "graphics.h"

static int game_active = 0;
static Game* current_game = 0;
static uint32_t* saved_framebuffer = 0;
static uint32_t saved_width = 0;
static uint32_t saved_height = 0;
static uint32_t saved_pitch = 0;

// Function to restore original framebuffer
extern void graphics_init(uint32_t addr, uint32_t pitch, uint32_t w, uint32_t h, uint8_t bpp);
extern uint32_t fb_width_get(void);
extern uint32_t fb_height_get(void);
extern uint32_t* fb_get(void);

static void save_video_state(void) {
    saved_width = fb_width_get();
    saved_height = fb_height_get();
    saved_pitch = saved_width * 4; // Assume 32bpp
    
    // Save framebuffer content
    uint32_t size = saved_width * saved_height * 4;
    saved_framebuffer = malloc(size);
    if (saved_framebuffer) {
        memcpy(saved_framebuffer, fb_get(), size);
    }
}

static void restore_video_state(void) {
    // Restore original framebuffer using GRUB's info
    // This would need the original framebuffer address from boot
    // For now, just reinitialize graphics
    if (saved_framebuffer) {
        memcpy(fb_get(), saved_framebuffer, saved_width * saved_height * 4);
        free(saved_framebuffer);
        saved_framebuffer = 0;
    }
}

void gamemode_enter(Game* game) {
    if (!game) return;
    
    game_active = 1;
    current_game = game;
    
    print("Entering game mode: ");
    print(game->name);
    print("\n");
    print("Press ESC to exit\n");
    
    // Save current video state
    save_video_state();
    
    if (game->init) game->init();
    if (game->run) game->run();
    if (game->cleanup) game->cleanup();
    
    // Restore video state
    restore_video_state();
    
    game_active = 0;
    current_game = 0;
    
    print("Exiting game mode\n");
}

void gamemode_exit(void) {
    game_active = 0;
}

int gamemode_active(void) {
    return game_active;
}