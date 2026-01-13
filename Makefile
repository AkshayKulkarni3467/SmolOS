CC = gcc
AS = as
CFLAGS = -nostdlib -fno-builtin -fno-stack-protector -O2 -Wall -Wextra -m32 -Iinclude
ASFLAGS = --32

DISK_IMG = disk.img
DISK_IMG_SIZE = 10

OBJS = bootloader/sos_boot.o \
       bootloader/sos_idt.o \
       kernel/sos_kernel.o \
       applications/sos_calculator.o \
       applications/sos_filemanager.o \
	   applications/sos_todolist.o \
	   applications/sos_artgallery.o \
	   applications/sos_gui.o \
       drivers/rtc/sos_rtc.o \
       drivers/ata/sos_ata.o \
       drivers/fat/sos_fat16.o \
       drivers/timer/sos_pit.o \
       drivers/interrupts/sos_idt.o \
       io/sos_io.o \
       io/sos_shell.o \
       commands/sos_cmds.o \
       drivers/input/sos_keyboard.o \
	   drivers/input/sos_mouse.o \
       drivers/video/sos_vga.o \
       drivers/video/sos_vgraphics.o \
       libraries/sos_string.o \
       libraries/sos_memory.o \
       libraries/sos_stdio.o \
	   libraries/sos_div64.o \
	   games/sos_snake.o 

all: SmolOS.bin

SmolOS.bin: $(OBJS)
	$(CC) -m32 -nostdlib -o SmolOS.bin $(OBJS) -T linker/sos_linker.ld

bootloader/sos_boot.o: bootloader/sos_boot.s
	$(CC) $(CFLAGS) -c bootloader/sos_boot.s -o bootloader/sos_boot.o

bootloader/sos_idt.o: bootloader/sos_idt.s
	$(AS) $(ASFLAGS) bootloader/sos_idt.s -o bootloader/sos_idt.o

kernel/sos_kernel.o: kernel/sos_kernel.c include/sos_vga.h include/sos_idt.h
	$(CC) $(CFLAGS) -c kernel/sos_kernel.c -o kernel/sos_kernel.o

libraries/sos_string.o: include/sos_string.h libraries/sos_string.c
	$(CC) $(CFLAGS) -c libraries/sos_string.c -o libraries/sos_string.o

libraries/sos_memory.o: include/sos_memory.h libraries/sos_memory.c
	$(CC) $(CFLAGS) -c libraries/sos_memory.c -o libraries/sos_memory.o

libraries/sos_stdio.o: include/sos_stdio.h libraries/sos_stdio.c
	$(CC) $(CFLAGS) -c libraries/sos_stdio.c -o libraries/sos_stdio.o

libraries/sos_div64.o: include/sos_div64.h libraries/sos_div64.c
	$(CC) $(CFLAGS) -c libraries/sos_div64.c -o libraries/sos_div64.o

io/sos_io.o: include/sos_io.h io/sos_io.c
	$(CC) $(CFLAGS) -c io/sos_io.c -o io/sos_io.o

io/sos_shell.o: include/sos_shell.h io/sos_shell.c
	$(CC) $(CFLAGS) -c io/sos_shell.c -o io/sos_shell.o

commands/sos_cmds.o: include/sos_cmds.h commands/sos_cmds.c
	$(CC) $(CFLAGS) -c commands/sos_cmds.c -o commands/sos_cmds.o

games/sos_snake.o: include/sos_snake.h games/sos_snake.c
	$(CC) $(CFLAGS) -c games/sos_snake.c -o games/sos_snake.o

applications/sos_filemanager.o: include/sos_filemanager.h applications/sos_filemanager.c
	$(CC) $(CFLAGS) -c applications/sos_filemanager.c -o applications/sos_filemanager.o

applications/sos_calculator.o: include/sos_calculator.h applications/sos_calculator.c
	$(CC) $(CFLAGS) -c applications/sos_calculator.c -o applications/sos_calculator.o

applications/sos_todolist.o: include/sos_todolist.h applications/sos_todolist.c
	$(CC) $(CFLAGS) -c applications/sos_todolist.c -o applications/sos_todolist.o

applications/sos_artgallery.o: include/sos_artgallery.h applications/sos_artgallery.c
	$(CC) $(CFLAGS) -c applications/sos_artgallery.c -o applications/sos_artgallery.o

applications/sos_gui.o: include/sos_gui.h applications/sos_gui.c
	$(CC) $(CFLAGS) -c applications/sos_gui.c -o applications/sos_gui.o

drivers/input/sos_keyboard.o: include/sos_keyboard.h drivers/input/sos_keyboard.c
	$(CC) $(CFLAGS) -c drivers/input/sos_keyboard.c -o drivers/input/sos_keyboard.o

drivers/input/sos_mouse.o: include/sos_mouse.h drivers/input/sos_mouse.c
	$(CC) $(CFLAGS) -c drivers/input/sos_mouse.c -o drivers/input/sos_mouse.o

drivers/timer/sos_pit.o: include/sos_pit.h drivers/timer/sos_pit.c
	$(CC) $(CFLAGS) -c drivers/timer/sos_pit.c -o drivers/timer/sos_pit.o

drivers/video/sos_vga.o: include/sos_vga.h drivers/video/sos_vga.c
	$(CC) $(CFLAGS) -c drivers/video/sos_vga.c -o drivers/video/sos_vga.o

drivers/video/sos_vgraphics.o: include/sos_vgraphics.h drivers/video/sos_vgraphics.c
	$(CC) $(CFLAGS) -c drivers/video/sos_vgraphics.c -o drivers/video/sos_vgraphics.o

drivers/fat/sos_fat16.o: include/sos_fat16.h drivers/fat/sos_fat16.c
	$(CC) $(CFLAGS) -c drivers/fat/sos_fat16.c -o drivers/fat/sos_fat16.o

drivers/ata/sos_ata.o: include/sos_ata.h drivers/ata/sos_ata.c
	$(CC) $(CFLAGS) -c drivers/ata/sos_ata.c -o drivers/ata/sos_ata.o

drivers/rtc/sos_rtc.o: include/sos_rtc.h drivers/rtc/sos_rtc.c
	$(CC) $(CFLAGS) -c drivers/rtc/sos_rtc.c -o drivers/rtc/sos_rtc.o

drivers/interrupts/sos_idt.o: include/sos_idt.h drivers/interrupts/sos_idt.c
	$(CC) $(CFLAGS) -c drivers/interrupts/sos_idt.c -o drivers/interrupts/sos_idt.o

create-img: 
	qemu-img create -f raw disk.img 10M

hexdump-img:
	hexdump -C disk.img 

clean:
	rm -f applications/*.o commands/*.o bootloader/*.o kernel/*.o drivers/*/*.o libraries/*.o io/*.o games/*.o SmolOS.bin 
	rm -f *-test

run: SmolOS.bin
	@if [ ! -f $(DISK_IMG) ]; then \
		echo "$(DISK_IMG) not found. Creating FAT16 disk..."; \
		qemu-img create -f raw $(DISK_IMG) $(DISK_IMG_SIZE)M; \
	fi
	qemu-system-i386 -kernel SmolOS.bin -drive file=disk.img,format=raw,if=ide,index=0

.PHONY: all clean run 