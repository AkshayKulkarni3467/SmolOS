CC = gcc
CFLAGS = -nostdlib -fno-builtin -fno-stack-protector -O2 -Wall -Wextra -m32 -Iinclude

OBJS = bootloader/sos_boot.o kernel/sos_kernel.o vga/sos_vga.o drivers/sos_string.o drivers/sos_memory.o drivers/sos_stdio.o

all: SmolOS.bin

SmolOS.bin: $(OBJS)
	$(CC) -m32 -nostdlib -o SmolOS.bin $(OBJS) -T linker/sos_linker.ld

bootloader/sos_boot.o: bootloader/sos_boot.s
	$(CC) $(CFLAGS) -c bootloader/sos_boot.s -o bootloader/sos_boot.o

kernel/sos_kernel.o: kernel/sos_kernel.c
	$(CC) $(CFLAGS) -c kernel/sos_kernel.c -o kernel/sos_kernel.o

drivers/sos_string.o: include/sos_string.h drivers/sos_string.c
	$(CC) $(CFLAGS) -c drivers/sos_string.c -o drivers/sos_string.o

drivers/sos_memory.o: include/sos_memory.h drivers/sos_memory.c
	$(CC) $(CFLAGS) -c drivers/sos_memory.c -o drivers/sos_memory.o

drivers/sos_studio.o: include/sos_studio.h drivers/sos_studio.c
	$(CC) $(CFLAGS) -c drivers/sos_studio.c -o drivers/sos_studio.o

vga/sos_vga.o : include/sos_vga.h vga/sos_vga.c
	$(CC) $(CFLAGS) -c vga/sos_vga.c -o vga/sos_vga.o

vga-test: include/sos_vga.h vga/sos_vga.c
	gcc vga/sos_vga.c -fno-builtin -DSMOLOS_VGA_TEST -o vga-test -Iinclude

string-test: drivers/sos_string.c 
	gcc drivers/sos_string.c -fno-builtin -DSMOLOS_STRING_TEST -o string-test -Iinclude

kernel-test: kernel/sos_kernel.c vga/sos_vga.o
	gcc kernel/sos_kernel.c -fno-builtin -DSMOLOS_KERNEL_TEST -o kernel-test vga/sos_vga.o -Iinclude -m32

clean:
	rm -f bootloader/*.o kernel/*.o vga/*.o drivers/*.o SmolOS.bin 
	rm -f vga-test string-test kernel-test 

run: SmolOS.bin
	qemu-system-i386 -kernel SmolOS.bin

.PHONY: all clean run