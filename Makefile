CC = gcc
CFLAGS = -nostdlib -fno-builtin -fno-stack-protector -O2 -Wall -Wextra -m32 -Iinclude

OBJS = bootloader/sos_boot.o kernel/sos_kernel.o drivers/timer/sos_pit.o io/sos_io.o drivers/input/sos_keyboard.o drivers/video/sos_vga.o drivers/video/sos_vgraphics.o libraries/sos_string.o libraries/sos_memory.o libraries/sos_stdio.o

all: SmolOS.bin

SmolOS.bin: $(OBJS)
	$(CC) -m32 -nostdlib -o SmolOS.bin $(OBJS) -T linker/sos_linker.ld

bootloader/sos_boot.o: bootloader/sos_boot.s
	$(CC) $(CFLAGS) -c bootloader/sos_boot.s -o bootloader/sos_boot.o

kernel/sos_kernel.o: kernel/sos_kernel.c include/sos_vga.h
	$(CC) $(CFLAGS) -c kernel/sos_kernel.c -o kernel/sos_kernel.o

libraries/sos_string.o: include/sos_string.h libraries/sos_string.c
	$(CC) $(CFLAGS) -c libraries/sos_string.c -o libraries/sos_string.o

libraries/sos_memory.o: include/sos_memory.h libraries/sos_memory.c
	$(CC) $(CFLAGS) -c libraries/sos_memory.c -o libraries/sos_memory.o

libraries/sos_stdio.o: include/sos_stdio.h libraries/sos_stdio.c
	$(CC) $(CFLAGS) -c libraries/sos_stdio.c -o libraries/sos_stdio.o

io/sos_io.o: include/sos_io.h io/sos_io.c
	$(CC) $(CFLAGS) -c io/sos_io.c -o io/sos_io.o

drivers/input/sos_keyboard.o: include/sos_keyboard.h drivers/input/sos_keyboard.c
	$(CC) $(CFLAGS) -c drivers/input/sos_keyboard.c -o drivers/input/sos_keyboard.o

drivers/timer/sos_pit.o: include/sos_pit.h drivers/timer/sos_pit.c
	$(CC) $(CFLAGS) -c drivers/timer/sos_pit.c -o drivers/timer/sos_pit.o

drivers/video/sos_vga.o: include/sos_vga.h drivers/video/sos_vga.c
	$(CC) $(CFLAGS) -c drivers/video/sos_vga.c -o drivers/video/sos_vga.o

drivers/video/sos_vgraphics.o: include/sos_vgraphics.h drivers/video/sos_vgraphics.c
	$(CC) $(CFLAGS) -c drivers/video/sos_vgraphics.c -o drivers/video/sos_vgraphics.o

vga-test: include/sos_vga.h drivers/video/sos_vga.c
	gcc drivers/video/sos_vga.c -fno-builtin -DSMOLOS_VGA_TEST -o vga-test -Iinclude
	./vga-test

vgraphics-test: include/sos_vgraphics.h drivers/video/sos_vgraphics.c drivers/video/sos_vga.o
	gcc drivers/video/sos_vgraphics.c -fno-builtin -DSMOLOS_VGRAPHICS_TEST -o vgraphics-test drivers/video/sos_vga.o -Iinclude -m32

keyboard-test: include/sos_keyboard.h drivers/input/sos_keyboard.c 
	gcc drivers/input/sos_keyboard.c -fno-builtin -DSMOLOS_KEYBOARD_TEST -o keyboard-test -Iinclude 

string-test: libraries/sos_string.c include/sos_string.h
	gcc libraries/sos_string.c -fno-builtin -DSMOLOS_STRING_TEST -o string-test -Iinclude
	./string-test

memory-test: libraries/sos_memory.c include/sos_memory.h
	gcc libraries/sos_memory.c -fno-builtin -DSMOLOS_MEMORY_TEST -o memory-test -Iinclude
	./memory-test

kernel-test: kernel/sos_kernel.c include/sos_vga.h drivers/video/sos_vga.o drivers/input/sos_keyboard.o drivers/video/sos_vgraphics.o
	gcc kernel/sos_kernel.c -fno-builtin -DSMOLOS_KERNEL_TEST -o kernel-test drivers/input/sos_keyboard.o drivers/video/sos_vgraphics.o drivers/video/sos_vga.o -Iinclude -m32
	./kernel-test

test-all: vga-test vgraphics-test string-test memory-test keyboard-test kernel-test
	@echo ""
	@echo "=== All Tests Completed ==="

clean:
	rm -f bootloader/*.o kernel/*.o drivers/video/*.o drivers/*.o drivers/input/*.o drivers/timer/*.o io/*.o libraries/*.o SmolOS.bin 
	rm -f *-test

run: SmolOS.bin
	qemu-system-i386 -kernel SmolOS.bin

.PHONY: all clean run test-all vga-test string-test memory-test kernel-test