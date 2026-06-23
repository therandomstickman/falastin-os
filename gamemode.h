#ifndef GAMEMODE_H
#define GAMEMODE_H

typedef struct {
    const char* name;
    void (*init)(void);
    void (*run)(void);
    void (*cleanup)(void);
} Game;

void gamemode_enter(Game* game);
void gamemode_exit(void);
int  gamemode_active(void);

#endif