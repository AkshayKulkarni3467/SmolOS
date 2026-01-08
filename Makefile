CC = gcc
CFLAGS = -nostdlib -fno-builtin -fno-stack-protector -O2 -Wall -Wextra -m32 -Iinclude

OBJS = bootloader/sos_boot.o kernel/sos_kernel.o keyboard/sos_keyboard.o vga/sos_vga.o vga/sos_vgraphics.o drivers/sos_string.o drivers/sos_memory.o drivers/sos_stdio.o

all: SmolOS.bin

SmolOS.bin: $(OBJS)
	$(CC) -m32 -nostdlib -o SmolOS.bin $(OBJS) -T linker/sos_linker.ld

bootloader/sos_boot.o: bootloader/sos_boot.s
	$(CC) $(CFLAGS) -c bootloader/sos_boot.s -o bootloader/sos_boot.o

kernel/sos_kernel.o: kernel/sos_kernel.c include/sos_vga.h
	$(CC) $(CFLAGS) -c kernel/sos_kernel.c -o kernel/sos_kernel.o

drivers/sos_string.o: include/sos_string.h drivers/sos_string.c
	$(CC) $(CFLAGS) -c drivers/sos_string.c -o drivers/sos_string.o

drivers/sos_memory.o: include/sos_memory.h drivers/sos_memory.c
	$(CC) $(CFLAGS) -c drivers/sos_memory.c -o drivers/sos_memory.o

drivers/sos_stdio.o: include/sos_stdio.h drivers/sos_stdio.c
	$(CC) $(CFLAGS) -c drivers/sos_stdio.c -o drivers/sos_stdio.o

keyboard/sos_keyboard.o: include/sos_keyboard.h keyboard/sos_keyboard.c
	$(CC) $(CFLAGS) -c keyboard/sos_keyboard.c -o keyboard/sos_keyboard.o

vga/sos_vga.o: include/sos_vga.h vga/sos_vga.c
	$(CC) $(CFLAGS) -c vga/sos_vga.c -o vga/sos_vga.o

vga/sos_vgraphics.o: include/sos_vgraphics.h vga/sos_vgraphics.c
	$(CC) $(CFLAGS) -c vga/sos_vgraphics.c -o vga/sos_vgraphics.o

vga-test: include/sos_vga.h vga/sos_vga.c
	gcc vga/sos_vga.c -fno-builtin -DSMOLOS_VGA_TEST -o vga-test -Iinclude
	./vga-test

string-test: drivers/sos_string.c include/sos_string.h
	gcc drivers/sos_string.c -fno-builtin -DSMOLOS_STRING_TEST -o string-test -Iinclude
	./string-test

memory-test: drivers/sos_memory.c include/sos_memory.h
	gcc drivers/sos_memory.c -fno-builtin -DSMOLOS_MEMORY_TEST -o memory-test -Iinclude
	./memory-test

kernel-test: kernel/sos_kernel.c include/sos_vga.h vga/sos_vga.o
	gcc kernel/sos_kernel.c -fno-builtin -DSMOLOS_KERNEL_TEST -o kernel-test vga/sos_vga.o -Iinclude -m32
	./kernel-test

test-all: vga-test string-test memory-test kernel-test
	@echo ""
	@echo "=== All Tests Completed ==="

clean:
	rm -f bootloader/*.o kernel/*.o vga/*.o drivers/*.o keyboard/*.o SmolOS.bin 
	rm -f *-test

run: SmolOS.bin
	qemu-system-i386 -kernel SmolOS.bin

.PHONY: all clean run test-all vga-test string-test memory-test kernel-test