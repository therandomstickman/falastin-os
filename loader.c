#include "loader.h"
#include "fs.h"
#include "screen.h"

#define PROGRAM_LOAD_ADDRESS 0x8000

int load_and_execute(const char* filename)
{
    print("\n=== Loading Program ===\n");
    print("File: ");
    print(filename);
    print("\n");
    
    uint8_t buffer[32768];
    int size = fs_load_program(filename, buffer, sizeof(buffer));
    
    if (size <= 0) {
        print("Failed to load program\n");
        return -1;
    }
    
    print("Loaded ");
    print_int(size);
    print(" bytes\n");
    
    // Copy to execution address
    for (int i = 0; i < size; i++) {
        ((uint8_t*)PROGRAM_LOAD_ADDRESS)[i] = buffer[i];
    }
    
    print("Executing program...\n\n");
    
    // Execute!
    void (*program)(void) = (void (*)(void))PROGRAM_LOAD_ADDRESS;
    program();
    
    print("\nProgram finished\n");
    return 0;
}