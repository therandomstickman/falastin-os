# FalastinOS

A buggy OS written in C and x86 assembly.

**Disclamer**: most code here is written by AI, I don't know assembly or C, only Python at an intermediate level.

## Features

- **Custom Shell** with command parsing and argument support (based off DOS)
- **Text Editor** - Create and edit files directly in the OS
- **RAM-based Filesystem** - Create, read, write, delete files
- **Keyboard Driver** - Supports arrow keys, Shift, and Ctrl modifiers
- **Hardware Cursor** - Visual feedback for typing

## Commands

| Command | Description |
|---------|-------------|
| `help` | Show available commands |
| `ls` | List files in the filesystem |
| `touch <file>` | Create a new empty file |
| `write <file> <text>` | Write text to a file |
| `cat <file>` | Display file contents |
| `edit <file>` | Open the text editor |
| `rm <file>` | Delete a file |
| `clear` | Clear the screen |
| `info` | Show system information |
| `about` | About FalastinOS |
| `debug` | Show filesystem debug info |
| `bugs` | Display known issues |

## Editor Shortcuts

| Key | Action |
|-----|--------|
| `Ctrl+S` | Save file |
| `Ctrl+Q` | Quit editor |
| `Arrow Keys` | Navigate cursor |
| `Enter` | New line |
| `Backspace` | Delete character |

## Building and Running

### Prerequisites

```bash
sudo apt-get install gcc nasm qemu-system-x86 grub-pc-bin xorriso
```
### Build Commands
```bash
make clean
make run
qemu-img create -f raw disk.img 10M
```
**Note: The last command here is not neccesary as the filesystem is RAM based.**
## Known Bugs
Opening edit twice (if you wrote something the first time) will cause it to break, as the cursor gets stuck at the first character and makes
you unable to delete what you already wrote (this also happens if you use the "write" command the first time)
## Technical Details
Architecture: x86 32-bit protected mode
Bootloader: GRUB Multiboot compliant
Memory: Custom memory management coming soon
Interrupts: IDT with keyboard IRQ handling
## Acknowledgments
Me, myself and I.
## Why?
Only God knows.
