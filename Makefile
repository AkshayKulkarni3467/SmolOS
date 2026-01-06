CC = gcc
CFLAGS = -nostdlib -fno-builtin -fno-stack-protector -O2 -Wall -Wextra -m32 -Ivga

OBJS = bootloader/sos_boot.o kernel/sos_kernel.o vga/sos_vga.o

all: SmolOS.bin

SmolOS.bin: $(OBJS)
	$(CC) -m32 -nostdlib -o SmolOS.bin $(OBJS) -T linker/sos_linker.ld

bootloader/sos_boot.o: bootloader/sos_boot.s
	$(CC) $(CFLAGS) -c bootloader/sos_boot.s -o bootloader/sos_boot.o

kernel/sos_kernel.o: kernel/sos_kernel.c
	$(CC) $(CFLAGS) -c kernel/sos_kernel.c -o kernel/sos_kernel.o

vga/sos_vga.o : vga/sos_vga.h vga/sos_vga.c
	$(CC) $(CFLAGS) -c vga/sos_vga.c -o vga/sos_vga.o

clean:
	rm -f bootloader/*.o kernel/*.o vga/*.o SmolOS.bin

run: SmolOS.bin
	qemu-system-i386 -kernel SmolOS.bin

.PHONY: all clean run