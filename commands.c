#include "commands.h"
#include "screen.h"
#include "fs.h"
#include "loader.h"
#include "editor.h"
#include "ata.h"
#include "diskfs.h"
#include "malloc.h"
#include "timer.h"
#include "net.h"
#include "rtl8139.h"   // Add this line
#include <string.h> 
#include "lwip_port.h"  // Add this line if not already
extern void* saved_mb_info;

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
static void cmd_bugs(int argc, char** argv);
static void cmd_mkdir(int argc, char** argv);
static void cmd_cd(int argc, char** argv);
static void cmd_pwd(int argc, char** argv);
static void cmd_rmdir(int argc, char** argv);
static void cmd_gui(int argc, char** argv);
static void cmd_fbgui(int argc, char** argv);
static void cmd_sync(int argc, char** argv);
static void cmd_reboot(int argc, char** argv);
static void cmd_testfs(int argc, char** argv);
static void cmd_checkfs(int argc, char** argv);
static void cmd_rawtest(int argc, char** argv);
static void cmd_heap(int argc, char** argv);
static void cmd_uptime(int argc, char** argv);
static void cmd_edit(int argc, char** argv);
static void cmd_doom(int argc, char** argv);
static void cmd_gamemode(int argc, char** argv);
static void cmd_snake(int argc, char** argv);
static void cmd_net(int argc, char** argv);
static void cmd_ping(int argc, char** argv);
static void cmd_loopback(int argc, char** argv);
static void cmd_netdebug(int argc, char** argv);
static void cmd_rxcheck(int argc, char** argv);
static void cmd_broadcast(int argc, char** argv);
static void cmd_arpcache(int argc, char** argv);
static void cmd_addarp(int argc, char** argv);
//static void cmd_http(int argc, char** argv);
static void cmd_setip(int argc, char** argv);
static void cmd_arpset(int argc, char** argv);
static void cmd_lwip(int argc, char** argv);
static void cmd_lwiptest(int argc, char** argv);
static void cmd_browse(int argc, char** argv);
static void cmd_httpget(int argc, char** argv);
static void cmd_showhttp(int argc, char** argv);

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
    {"bugs", cmd_bugs, "bugs - Display known issues"},
    {"mkdir", cmd_mkdir, "mkdir <dir> - Create directory"},
    {"cd", cmd_cd, "cd <dir> - Change directory"},
    {"pwd", cmd_pwd, "pwd - Print working directory"},
    {"rmdir", cmd_rmdir, "rmdir <dir> - Remove empty directory"},
    {"gui", cmd_gui, "gui - Switch to graphical mode"},
    {"fbgui", cmd_fbgui, "fbgui - Enter framebuffer GUI mode"},
    {"sync", cmd_sync, "sync - Save filesystem to disk"},
    {"reboot", cmd_reboot, "reboot - Reboot the system"},
    {"testfs", cmd_testfs, "testfs - Create test file"},
    {"checkfs", cmd_checkfs, "checkfs - Check if file persists"},
    {"rawtest", cmd_rawtest, "rawtest - Test raw ATA sector read/write"},
    {"heap", cmd_heap, "heap - Show heap usage"},
    {"uptime", cmd_uptime, "uptime - Show system uptime"},
    {"edit", cmd_edit, "edit <filename> - Open the file editor"},
    {"doom", cmd_doom, "doom - Play DOOM!"},
    {"gamemode", cmd_gamemode, "gamemode - Mini emulation layer for old games"},
    {"snake", cmd_snake, "snake - play snake!"},
    {"net", cmd_net, "net - Initialize network and show MAC"},
    {"ping", cmd_ping, "ping <ip> - Send ICMP echo request"},
    {"loopback", cmd_loopback, "loopback - Test network stack with loopback ping"},
    {"netdebug", cmd_netdebug, "netdebug - Show network debug info"},
    {"rxcheck", cmd_rxcheck, "rxcheck - Check for received packets"},
    {"broadcast", cmd_broadcast, "broadcast - Send ARP broadcast"},
    {"arpcache", cmd_arpcache, "arpcache - Show ARP cache"},
    {"addarp", cmd_addarp, "addarp <ip> - Send ARP request"},
    //{"http", cmd_http, "http <ip> - Test HTTP"},
    {"setip", cmd_setip, "setip <ip> - Set IP"},
    {"arpset", cmd_arpset, "arpset <ip> <mac> - Manually set ARP entry"},
    {"lwip", cmd_lwip, "lwip - Initialize lwIP TCP/IP stack"},
    {"lwiptest", cmd_lwiptest, "lwiptest - Test lwIP stack"},
    {"browse", cmd_browse, "browse <url> - Browse a website"},
    {"httpget", cmd_httpget, "httpget <host> [path] - HTTP GET request"},
    {"showhttp", cmd_showhttp, "showhttp - Show HTTP response"},
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

static void cmd_gui(int argc, char** argv)
{
    (void)argc; (void)argv;
    
    // Need to include the multiboot struct definition here
    // For now, just call brickwm directly
    print("Starting GUI mode...\n");
    print("Press ESC to return to shell.\n");
    
    // Clear screen before going to GUI
    clear_screen();
    
    // Initialize graphics with framebuffer info
    // For now, use a simple test
    extern void brickwm_run(void);
    brickwm_run();
    
    // After returning, clear screen and show shell again
    clear_screen();
    print("Returned to shell mode.\n");
}

static void cmd_fbgui(int argc, char** argv)
{
    (void)argc; (void)argv;
    extern void fbgui_run(void);
    fbgui_run();
}

static void cmd_sync(int argc, char** argv) {
    (void)argc; (void)argv;
    print("=== SYNC START ===\n");
    
    // List files before saving
    print("Files in memory:\n");
    fs_list(".");
    
    extern void diskfs_save(void);
    diskfs_save();
    
    print("=== SYNC END ===\n");
}

static void cmd_reboot(int argc, char** argv) {
    (void)argc; (void)argv;
    print("Exiting QEMU...\n");
    // Write to QEMU's isa-debug-exit device
    asm volatile (
        "movw $0x501, %%dx\n"
        "movl $0x01, %%eax\n"
        "outl %%eax, %%dx\n"
        : : : "eax", "edx"
    );
    // If that fails, try triple fault
    asm volatile (
        "cli\n"
        "hlt\n"
    );
    for(;;);
}

static void cmd_testfs(int argc, char** argv) {
    (void)argc; (void)argv;
    
    print("Testing filesystem persistence:\n");
    print("Creating test file...\n");
    fs_write("persist_test.txt", "Hello persistence!", 19);
    print("File created. Type 'sync' to save, then reboot.\n");
    print("After reboot, type 'checkfs' to verify.\n");
}

static void cmd_checkfs(int argc, char** argv) {
    (void)argc; (void)argv;
    
    char buffer[100];
    int bytes = fs_read("persist_test.txt", buffer, 99);
    if (bytes > 0) {
        buffer[bytes] = '\0';
        print("File found! Content: ");
        print(buffer);
        print("\nPERSISTENCE WORKS!\n");
    } else {
        print("File not found. Persistence failed.\n");
    }
}

static void cmd_rawtest(int argc, char** argv) {
    (void)argc; (void)argv;
    uint8_t buf[512];
    
    print("=== ATA Raw Sector Test ===\n");
    
    // Check if ATA drive present
    if (!ata_drive_present()) {
        print("No ATA drive detected!\n");
        return;
    }
    
    // Fill test pattern
    for (int i = 0; i < 512; i++) buf[i] = i & 0xFF;
    
    print("Writing test pattern to sector 300...\n");
    if (ata_write_sector(300, buf) != 0) {
        print("Write failed!\n");
        return;
    }
    
    // Clear buffer
    for (int i = 0; i < 512; i++) buf[i] = 0;
    
    print("Reading back from sector 300...\n");
    if (ata_read_sector(300, buf) != 0) {
        print("Read failed!\n");
        return;
    }
    
    // Verify
    int ok = 1;
    for (int i = 0; i < 512; i++) {
        if (buf[i] != (i & 0xFF)) {
            print("Mismatch at byte ");
            print_int(i);
            print("\n");
            ok = 0;
            break;
        }
    }
    
    if (ok) {
        print("ATA raw test PASSED! Disk read/write works.\n");
    } else {
        print("ATA raw test FAILED!\n");
    }
}

static void cmd_heap(int argc, char** argv) {
    (void)argc; (void)argv;
    
    // Test malloc
    void* a = malloc(1024);
    void* b = malloc(256);
    void* c = malloc(512);
    
    malloc_debug();  // should show 3 used blocks
    
    free(b);
    malloc_debug();  // middle block should be free
    
    void* d = malloc(128);  // should reuse b's space
    malloc_debug();
    
    free(a);
    free(c);
    free(d);
    malloc_debug();  // should be back to 1 block, all free
}

static void cmd_uptime(int argc, char** argv) {
    (void)argc; (void)argv;
    print_fmt("Uptime: %d seconds (%d ticks)\n",
              timer_ticks() / 100, timer_ticks());
}

static void cmd_edit(int argc, char** argv) {
    if (argc < 2) {
        print("Usage: edit <filename>\n");
        return;
    }

    editor_open(argv[1]);
    print("Returned from editor.\n");
}

static void cmd_doom(int argc, char** argv) {
    (void)argc; (void)argv;
    extern void doom_run(void);
    doom_run();
}

static void cmd_gamemode(int argc, char** argv) {
    (void)argc; (void)argv;
    print("Game Mode ready!\n");
    print("Games available: snake\n");
}

static void cmd_snake(int argc, char** argv) {
    (void)argc; (void)argv;
    extern void snake_start(void);
    snake_start();
}

static void cmd_net(int argc, char** argv) {
    (void)argc; (void)argv;
    extern void rtl8139_init(void);
    rtl8139_init();
    uint8_t mac[6];
    rtl8139_get_mac(mac);
    print("MAC: ");
    for (int i = 0; i < 6; i++) {
        print_int(mac[i]);
        if (i < 5) print(":");
    }
    print("\n");
}

static void cmd_ping(int argc, char** argv) {
    if (argc < 2) {
        print("Usage: ping <ip>\n");
        return;
    }
    extern void ping(uint32_t);
    extern uint32_t ip_str_to_addr(const char*);
    ping(ip_str_to_addr(argv[1]));
}

static void cmd_loopback(int argc, char** argv) {
    (void)argc; (void)argv;
    extern void net_test_loopback(void);
    net_test_loopback();
}

static void cmd_netdebug(int argc, char** argv) {
    (void)argc; (void)argv;
    extern void rtl8139_debug(void);
    rtl8139_debug();
}

static void cmd_rxcheck(int argc, char** argv) {
    (void)argc; (void)argv;
    print("Checking RX buffer...\n");
    for (int i = 0; i < 100; i++) {
        if (rtl8139_packet_available()) {
            print("Packet available at offset ");
            print_int(i);
            print("\n");
            uint8_t packet[2048];
            int len = rtl8139_receive_packet(packet, sizeof(packet));
            if (len > 0) {
                print("Received packet, len: ");
                print_int(len);
                print("\n");
            }
        }
    }
    print("RX check complete.\n");
}

static void cmd_broadcast(int argc, char** argv) {
    (void)argc; (void)argv;
    print("Sending broadcast ARP request...\n");
    extern void arp_send_request(uint32_t);
    arp_send_request(0xFFFFFFFF);
}

static void cmd_arpcache(int argc, char** argv) {
    (void)argc; (void)argv;
    extern ArpCacheEntry arp_cache[16];
    extern int arp_cache_count;
    
    print("ARP Cache:\n");
    for (int i = 0; i < arp_cache_count; i++) {
        if (arp_cache[i].valid) {
            char ip_buf[16];
            ip_addr_to_str(arp_cache[i].ip, ip_buf);
            print("  ");
            print(ip_buf);
            print(" -> ");
            for (int j = 0; j < 6; j++) {
                print_int(arp_cache[i].mac[j]);
                if (j < 5) print(":");
            }
            print("\n");
        }
    }
    if (arp_cache_count == 0) {
        print("  (empty)\n");
    }
}

static void cmd_addarp(int argc, char** argv) {
    if (argc < 2) {
        print("Usage: addarp <ip>\n");
        return;
    }
    extern void arp_send_request(uint32_t);
    arp_send_request(ip_str_to_addr(argv[1]));
}
/*
static void cmd_http(int argc, char** argv) {
    if (argc < 2) {
        print("Usage: http <ip>\n");
        return;
    }
    
    print("Sending HTTP GET request to ");
    print(argv[1]);
    print("\n");
    
    uint32_t dest_ip = ip_str_to_addr(argv[1]);
    uint8_t* dest_mac = arp_lookup(dest_ip);
    if (!dest_mac) {
        print("No ARP entry\n");
        return;
    }
    
    const char* http = "GET / HTTP/1.1\r\nHost: example.com\r\nConnection: close\r\n\r\n";
    uint32_t http_len = 0;
    while (http[http_len]) http_len++;
    
    // Build TCP SYN packet (simplified)
    uint8_t packet[512];
    EthHeader* eth = (EthHeader*)packet;
    IpHeader* ip = (IpHeader*)(packet + sizeof(EthHeader));
    
    memcpy(eth->dest_mac, dest_mac, 6);
    memcpy(eth->src_mac, our_mac, 6);
    eth->ethertype = htons(0x0800);
    
    uint32_t total_len = sizeof(IpHeader) + 20;  // TCP header
    ip->ver_ihl = 0x45;
    ip->tos = 0;
    ip->total_len = htons(total_len);
    ip->id = htons(0x5678);
    ip->flags_off = 0;
    ip->ttl = 64;
    ip->protocol = 6;  // TCP
    ip->src_ip = htonl(our_ip);
    ip->dest_ip = htonl(dest_ip);
    ip->checksum = 0;
    ip->checksum = net_checksum((uint16_t*)ip, sizeof(IpHeader));
    
    rtl8139_send_packet(packet, sizeof(EthHeader) + total_len);
    print("TCP SYN sent\n");
}
*/

static void cmd_setip(int argc, char** argv) {
    if (argc < 2) {
        print("Usage: setip <ip> [gateway] [mask]\n");
        return;
    }
    extern void net_set_static_ip(const char*, const char*, const char*);
    const char* gateway = (argc > 2) ? argv[2] : "10.0.2.2";
    const char* mask = (argc > 3) ? argv[3] : "255.255.255.0";
    net_set_static_ip(argv[1], gateway, mask);
}

static void cmd_arpset(int argc, char** argv) {
    if (argc < 3) {
        print("Usage: arpset <ip> <mac>\n");
        print("Example: arpset 192.168.1.1 00:11:22:33:44:55\n");
        return;
    }
    
    uint32_t ip = ip_str_to_addr(argv[1]);
    uint8_t mac[6];
    
    // Parse MAC address (format: xx:xx:xx:xx:xx:xx)
    for (int i = 0; i < 6; i++) {
        mac[i] = 0;
        if (argv[2][i*3] >= '0' && argv[2][i*3] <= '9') mac[i] = (argv[2][i*3] - '0') * 16;
        else if (argv[2][i*3] >= 'a' && argv[2][i*3] <= 'f') mac[i] = (argv[2][i*3] - 'a' + 10) * 16;
        else if (argv[2][i*3] >= 'A' && argv[2][i*3] <= 'F') mac[i] = (argv[2][i*3] - 'A' + 10) * 16;
        
        if (argv[2][i*3+1] >= '0' && argv[2][i*3+1] <= '9') mac[i] += argv[2][i*3+1] - '0';
        else if (argv[2][i*3+1] >= 'a' && argv[2][i*3+1] <= 'f') mac[i] += argv[2][i*3+1] - 'a' + 10;
        else if (argv[2][i*3+1] >= 'A' && argv[2][i*3+1] <= 'F') mac[i] += argv[2][i*3+1] - 'A' + 10;
    }
    
    // Add to ARP cache
    for (int i = 0; i < arp_cache_count; i++) {
        if (arp_cache[i].valid && arp_cache[i].ip == ip) {
            memcpy(arp_cache[i].mac, mac, 6);
            print("Updated ARP entry\n");
            return;
        }
    }
    if (arp_cache_count < 16) {
        arp_cache[arp_cache_count].ip = ip;
        memcpy(arp_cache[arp_cache_count].mac, mac, 6);
        arp_cache[arp_cache_count].valid = 1;
        arp_cache_count++;
        print("Added ARP entry\n");
    }
}

static void cmd_lwip(int argc, char** argv) {
    
    (void)argc;
    (void)argv;
    lwip_init_port();
    
}

static void cmd_lwiptest(int argc, char** argv) {
    (void)argc;
    (void)argv;
    extern void lwip_ping_test(void);
    lwip_ping_test();
}

static void cmd_browse(int argc, char** argv) {
    if (argc < 2) {
        print("Usage: browse <url>\n");
        print("Example: browse example.com\n");
        print("Example: browse http://example.com\n");
        return;
    }
    
    extern void browse(const char* url);
    browse(argv[1]);
}

static void cmd_httpget(int argc, char** argv) {
    if (argc < 2) {
        print("Usage: httpget <host> [path]\n");
        return;
    }
    
    extern void http_get(const char* host, const char* path);
    const char* path = (argc >= 3) ? argv[2] : "/";
    http_get(argv[1], path);
}

static void cmd_showhttp(int argc, char** argv) {
    (void)argc;
    (void)argv;
    extern void display_http_response(void);
    display_http_response();
}