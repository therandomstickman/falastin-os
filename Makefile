.PHONY: all iso run run-gui clean

all: run

# lwIP configuration
LWIP_DIR = lwip-2.1.3
LWIP_INC = -I$(LWIP_DIR)/src/include -I$(LWIP_DIR)/src/include/ipv4 -I./lwip_port

CFLAGS = -m32 -ffreestanding -nostdlib -fno-pie -fno-stack-protector -fno-builtin -Wall -Wextra $(LWIP_INC)
SOURCES = $(wildcard *.c *.h *.asm) font.psf linker.ld

# lwIP core sources
LWIP_CORE = $(LWIP_DIR)/src/core/init.c \
            $(LWIP_DIR)/src/core/mem.c \
            $(LWIP_DIR)/src/core/memp.c \
            $(LWIP_DIR)/src/core/pbuf.c \
            $(LWIP_DIR)/src/core/netif.c \
            $(LWIP_DIR)/src/core/ip.c \
            $(LWIP_DIR)/src/core/ipv4/ip4.c \
            $(LWIP_DIR)/src/core/ipv4/ip4_addr.c \
            $(LWIP_DIR)/src/core/ipv4/ip4_frag.c \
            $(LWIP_DIR)/src/core/ipv4/icmp.c \
            $(LWIP_DIR)/src/core/ipv4/autoip.c \
            $(LWIP_DIR)/src/core/ipv4/dhcp.c \
            $(LWIP_DIR)/src/core/ipv4/etharp.c \
            $(LWIP_DIR)/src/core/tcp.c \
            $(LWIP_DIR)/src/core/tcp_in.c \
            $(LWIP_DIR)/src/core/tcp_out.c \
            $(LWIP_DIR)/src/core/udp.c \
            $(LWIP_DIR)/src/core/def.c \
            $(LWIP_DIR)/src/core/timeouts.c \
            $(LWIP_DIR)/src/core/dns.c \
            $(LWIP_DIR)/src/core/inet_chksum.c \
            $(LWIP_DIR)/src/netif/ethernet.c

kernel.bin: $(SOURCES)
	nasm -f elf32 kernel_entry.asm -o kernel_entry.o
	nasm -f elf32 gdt.asm -o gdt.o
	nasm -f elf32 idt.asm -o idt.o
	nasm -f elf32 irq.asm -o irq.o
	objcopy -O elf32-i386 -B i386 -I binary font.psf font_psf.o
	gcc $(CFLAGS) -c kernel.c -o kernel.o
	gcc $(CFLAGS) -c screen.c -o screen.o
	gcc $(CFLAGS) -c cursor.c -o cursor.o
	gcc $(CFLAGS) -c shell.c -o shell.o
	gcc $(CFLAGS) -c commands.c -o commands.o
	gcc $(CFLAGS) -c keyboard.c -o keyboard.o
	gcc $(CFLAGS) -c interrupts.c -o interrupts.o
	gcc $(CFLAGS) -c pic.c -o pic.o
	gcc $(CFLAGS) -c fs.c -o fs.o
	gcc $(CFLAGS) -c ata.c -o ata.o
	gcc $(CFLAGS) -c diskfs.c -o diskfs.o
	gcc $(CFLAGS) -c loader.c -o loader.o
	gcc $(CFLAGS) -c editor.c -o editor.o
	gcc $(CFLAGS) -c graphics.c -o graphics.o
	gcc $(CFLAGS) -c brickwm.c -o brickwm.o
	gcc $(CFLAGS) -c fbgui.c -o fbgui.o
	gcc $(CFLAGS) -c font.c -o font.o
	gcc $(CFLAGS) -c mouse.c -o mouse.o
	gcc $(CFLAGS) -c cursor_gfx.c -o cursor_gfx.o
	gcc $(CFLAGS) -c term.c -o term.o
	gcc $(CFLAGS) -c term_io.c -o term_io.o
	gcc $(CFLAGS) -c progman.c -o progman.o
	gcc $(CFLAGS) -c libc.c -o libc.o
	gcc $(CFLAGS) -c malloc.c -o malloc.o
	gcc $(CFLAGS) -c timer.c -o timer.o
	gcc $(CFLAGS) -c fileman.c -o fileman.o
	gcc $(CFLAGS) -c gamemode.c -o gamemode.o
	gcc $(CFLAGS) -c snake.c -o snake.o
	gcc $(CFLAGS) -c doom_compat.c -o doom_compat.o
	gcc $(CFLAGS) -c doom_wrapper.c -o doom_wrapper.o
	gcc $(CFLAGS) -c rtl8139.c -o rtl8139.o
	gcc $(CFLAGS) -c pci.c -o pci.o
	gcc $(CFLAGS) -c net.c -o net.o
	gcc $(CFLAGS) -c lwip_port/sys_arch.c -o sys_arch.o
	gcc $(CFLAGS) -c lwip_port.c -o lwip_port.o
	gcc $(CFLAGS) -c html.c -o html.o
	gcc $(CFLAGS) -c dns_simple.c -o dns_simple.o
	# lwIP core - compile each file
	for f in $(LWIP_CORE); do \
		gcc $(CFLAGS) -c $$f -o $$(basename $$f .c).o; \
	done

	ld -m elf_i386 -T linker.ld \
		kernel_entry.o gdt.o idt.o irq.o \
		kernel.o screen.o cursor.o shell.o commands.o editor.o \
		keyboard.o interrupts.o pic.o fs.o ata.o diskfs.o loader.o graphics.o brickwm.o fbgui.o font_psf.o font.o mouse.o cursor_gfx.o term.o term_io.o progman.o libc.o malloc.o timer.o fileman.o gamemode.o snake.o doom_compat.o doom_wrapper.o rtl8139.o pci.o net.o sys_arch.o lwip_port.o html.o \
		init.o mem.o memp.o pbuf.o netif.o ip.o ip4.o ip4_addr.o ip4_frag.o icmp.o autoip.o dhcp.o etharp.o tcp.o tcp_in.o tcp_out.o udp.o def.o timeouts.o dns.o inet_chksum.o ethernet.o dns_simple.o \
		-o kernel.bin

iso: kernel.bin
	rm -rf iso falastinos.iso
	mkdir -p iso/boot/grub
	cp kernel.bin iso/boot/
	cp grub.cfg iso/boot/grub/
	grub-mkrescue -o falastinos.iso iso

run: kernel.bin iso
	@if [ ! -f falastinos.qcow2 ]; then \
		qemu-img create -f qcow2 falastinos.qcow2 10M > /dev/null 2>&1; \
		echo "Created disk image: falastinos.qcow2"; \
	fi
	qemu-system-i386 -cdrom falastinos.iso -m 32 \
		-machine pc \
		-nodefaults \
		-vga std \
		-display sdl \
		-drive file=falastinos.qcow2,format=qcow2,index=0,media=disk \
		-netdev user,id=net0,hostfwd=tcp::5555-:23 \
		-device rtl8139,netdev=net0

run-gui: kernel.bin iso
	@if [ ! -f falastinos.qcow2 ]; then \
		qemu-img create -f qcow2 falastinos.qcow2 10M > /dev/null 2>&1; \
		echo "Created disk image: falastinos.qcow2"; \
	fi
	qemu-system-i386 -cdrom falastinos.iso -hda falastinos.qcow2 -append "gui" -m 64

clean:
	rm -rf *.o kernel.bin iso falastinos.iso disk.img $(LWIP_DIR)/src/core/*.o $(LWIP_DIR)/src/core/ipv4/*.o $(LWIP_DIR)/src/netif/*.o