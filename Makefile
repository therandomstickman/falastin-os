all: kernel.bin iso run

CFLAGS = -m32 -ffreestanding -nostdlib -fno-pie -fno-stack-protector -fno-builtin -Wall -Wextra

kernel.bin:
	nasm -f elf32 kernel_entry.asm -o kernel_entry.o
	nasm -f elf32 gdt.asm -o gdt.o
	nasm -f elf32 idt.asm -o idt.o
	nasm -f elf32 irq.asm -o irq.o
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

	ld -m elf_i386 -T linker.ld \
		kernel_entry.o gdt.o idt.o irq.o \
		kernel.o screen.o cursor.o shell.o commands.o \
		keyboard.o interrupts.o pic.o fs.o ata.o diskfs.o loader.o editor.o \
		-o kernel.bin

iso:
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
	qemu-system-i386 -cdrom falastinos.iso -hda falastinos.qcow2 -m 32

clean:
	rm -rf *.o kernel.bin iso falastinos.iso disk.img