#include "doom_compat.h"
#include "screen.h"

// External Doom main function (after compilation)
extern int doom_main(int argc, char** argv);

void doom_run(void) {
    print("Starting DOOM...\n");
    print("Press ESC to exit\n");
    
    doom_video_init();
    
    // Prepare args
    char* argv[] = {"doom", 0};
    doom_main(1, argv);
    
    print("DOOM exited\n");
}