#include "commands.h"
#include "screen.h"
#include "fs.h"
#include "loader.h"
#include "editor.h"
#include "ata.h"
// Simple strcmp for command matching
static int strcmp_cmd(const char* a, const char* b)
{
    while (*a && *b && *a == *b) {
        a++;
        b++;
    }
    return *a - *b;
}
typedef struct {
    const char* name;
    void (*func)(int argc, char** argv);
    const char* help;
} Command;

static void cmd_help(int argc, char** argv);
static void cmd_info(int argc, char** argv);
static void cmd_about(int argc, char** argv);
static void cmd_clear(int argc, char** argv);
static void cmd_ls(int argc, char** argv);
static void cmd_touch(int argc, char** argv);
static void cmd_rm(int argc, char** argv);
static void cmd_write(int argc, char** argv);
static void cmd_cat(int argc, char** argv);
static void cmd_debug(int argc, char** argv);
static void cmd_edit(int argc, char** argv);
static void cmd_bugs(int argc, char** argv);
static void cmd_mkdir(int argc, char** argv);
static void cmd_cd(int argc, char** argv);
static void cmd_pwd(int argc, char** argv);
static void cmd_rmdir(int argc, char** argv);
static void cmd_testata(int argc, char** argv);

static Command commands[] = {
    {"help", cmd_help, "help - Show this help"},
    {"info", cmd_info, "info - Show system info"},
    {"about", cmd_about, "about - About this OS"},
    {"clear", cmd_clear, "clear - Clear screen"},
    {"ls", cmd_ls, "ls - List files"},
    {"touch", cmd_touch, "touch <filename> - Create a file"},
    {"rm", cmd_rm, "rm <filename> - Delete a file"},
    {"write", cmd_write, "write <filename> <text> - Write to a file"},
    {"cat", cmd_cat, "cat <filename> - Display file contents"},
    {"debug", cmd_debug, "debug - Show filesystem debug info"},
    {"edit", cmd_edit, "edit <filename> - Edit a file"},
    {"bugs", cmd_bugs, "bugs - Display known issues"},
    {"mkdir", cmd_mkdir, "mkdir <dir> - Create directory"},
    {"cd", cmd_cd, "cd <dir> - Change directory"},
    {"pwd", cmd_pwd, "pwd - Print working directory"},
    {"rmdir", cmd_rmdir, "rmdir <dir> - Remove empty directory"},
    {"testata", cmd_testata, "testata - Test ATA read/write"},

};

#define COMMAND_COUNT (sizeof(commands) / sizeof(commands[0]))

static int parse_command(char* input, char** argv)
{
    int argc = 0;
    while (*input) {
        while (*input == ' ') input++;
        if (*input == '\0') break;
        argv[argc++] = input;
        while (*input && *input != ' ') input++;
        if (*input) {
            *input = '\0';
            input++;
        }
    }
    return argc;
}

void execute_command(const char* input_str)
{
    if (input_str[0] == '\0') return;
    
    char buffer[256];
    int i;
    for (i = 0; input_str[i] && i < 255; i++) {
        buffer[i] = input_str[i];
    }
    buffer[i] = '\0';
    
    char* argv[16];
    int argc = parse_command(buffer, argv);
    
    if (argc == 0) return;
    
    for (unsigned int i = 0; i < COMMAND_COUNT; i++) {
        if (strcmp_cmd(argv[0], commands[i].name) == 0) {
            commands[i].func(argc, argv);
            return;
        }
    }
    
    print("Unknown command: ");
    print(argv[0]);
    print("\n");
}

static void cmd_help(int argc, char** argv)
{
    (void)argc; (void)argv;
    print("\n=== FalastinOS Commands ===\n");
    for (unsigned int i = 0; i < COMMAND_COUNT; i++) {
        print("  ");
        print(commands[i].help);
        print("\n");
    }
    print("\n");
}

static void cmd_info(int argc, char** argv)
{
    (void)argc; (void)argv;
    print("FalastinOS v0.1\n");
}

static void cmd_about(int argc, char** argv)
{
    (void)argc; (void)argv;
    print("a buggy os made with hopes and dreams\n");
}

static void cmd_clear(int argc, char** argv)
{
    (void)argc; (void)argv;
    clear_screen();
}

static void cmd_ls(int argc, char** argv)
{
    (void)argc; (void)argv;
    
    // If an argument is provided, use it as path, otherwise use current directory
    if (argc >= 2) {
        fs_list(argv[1]);
    } else {
        fs_list(".");  // Current directory
    }
}

static void cmd_touch(int argc, char** argv)
{
    if (argc < 2) {
        print("Usage: touch <filename>\n");
        return;
    }
    fs_create(argv[1]);
}

static void cmd_rm(int argc, char** argv)
{
    if (argc < 2) {
        print("Usage: rm <filename>\n");
        return;
    }
    fs_delete(argv[1]);
}

static void cmd_write(int argc, char** argv)
{
    if (argc < 3) {
        print("Usage: write <filename> <text>\n");
        return;
    }
    
    char text[512];
    int text_len = 0;
    for (int i = 2; i < argc; i++) {
        if (i > 2) text[text_len++] = ' ';
        for (int j = 0; argv[i][j]; j++) {
            text[text_len++] = argv[i][j];
        }
    }
    text[text_len] = '\0';
    fs_write(argv[1], text, text_len);
}

static void cmd_cat(int argc, char** argv)
{
    if (argc < 2) {
        print("Usage: cat <filename>\n");
        return;
    }
    
    char buffer[4096];
    int bytes = fs_read(argv[1], buffer, sizeof(buffer) - 1);
    if (bytes > 0) {
        buffer[bytes] = '\0';
        print(buffer);
        if (buffer[bytes-1] != '\n') print("\n");
    }
}

static void cmd_debug(int argc, char** argv)
{
    (void)argc; (void)argv;
    fs_debug();
}

static void cmd_edit(int argc, char** argv)
{
    if (argc < 2) {
        print("Usage: edit <filename>\n");
        return;
    }
    
    extern void editor_open(const char*);
    editor_open(argv[1]);
}

static void cmd_bugs(int argc, char** argv)
{
    (void)argc; (void)argv;
    print("List of known bugs:\n");
    print("1. Not a bug but if you are reading this and are somehow working on the OS, DO NOT TOUCH IRQ OR IDT\n");
    print(" THEY ARE HELD TOGETHER BY HOPES AND DREAMS\n");
    print("2. An actual bug this time, reopening the editor will completely break it after you already opened it. if you want to type something, you have one shot or use the write command and override it\n");
    print("3. If you expected this OS to have command history, well, it did at one point, but god knows what OBLITERATED IT\n");
    print("yeah thats all, sorry for the formatting btw to whoever reads this, i will never remove this\n");
    print("==========================FIXED BUGS=============================================================================\n");
    print("1. number two has been fixed\n");
}

static void cmd_mkdir(int argc, char** argv)
{
    if (argc < 2) {
        print("Usage: mkdir <directory>\n");
        return;
    }
    fs_mkdir(argv[1]);
}

static void cmd_cd(int argc, char** argv)
{
    if (argc < 2) {
        fs_cd("/");
    } else {
        fs_cd(argv[1]);
    }
}

static void cmd_pwd(int argc, char** argv)
{
    (void)argc; (void)argv;
    fs_pwd();
}

static void cmd_rmdir(int argc, char** argv)
{
    if (argc < 2) {
        print("Usage: rmdir <directory>\n");
        return;
    }
    fs_delete(argv[1]);  // Delete handles directory check
}

static void cmd_testata(int argc, char** argv)
{
    (void)argc; (void)argv;
    
    print("Testing ATA write/read...\n");
    
    uint8_t write_buf[512];
    uint8_t read_buf[512];
    
    // Fill test pattern
    for (int i = 0; i < 512; i++) {
        write_buf[i] = i % 256;
    }
    
    print("Writing to sector 200...\n");
    if (ata_write_sector(200, write_buf) != 0) {
        print("Write failed!\n");
        return;
    }
    
    print("Reading from sector 200...\n");
    if (ata_read_sector(200, read_buf) != 0) {
        print("Read failed!\n");
        return;
    }
    
    // Verify
    int match = 1;
    for (int i = 0; i < 512; i++) {
        if (write_buf[i] != read_buf[i]) {
            print("Mismatch at byte ");
            print_int(i);
            print("\n");
            match = 0;
            break;
        }
    }
    
    if (match) {
        print("ATA test PASSED! Disk works.\n");
    } else {
        print("ATA test FAILED!\n");
    }
}