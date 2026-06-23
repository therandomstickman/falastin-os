#!/bin/bash
# Download Doom source
wget https://github.com/id-Software/DOOM/archive/refs/heads/master.zip
unzip master.zip
cd DOOM-master/linuxdoom-1.10

# Use your cross-compiler (or just gcc -m32)
gcc -m32 -ffreestanding -nostdlib -fno-pie -c *.c
ar rcs libdoom.a *.o

# In your FalastinOS Makefile, add:
# gcc -m32 -c doom_wrapper.c -o doom_wrapper.o
# gcc -m32 -c doom_compat.c -o doom_compat.o
# And link with libdoom.a