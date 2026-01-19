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
	   applications/sos_musicplayer.o \
	   applications/sos_gui.o \
       drivers/rtc/sos_rtc.o \
       drivers/ata/sos_ata.o \
       drivers/fat/sos_fat16.o \
       drivers/timer/sos_pit.o \
       drivers/interrupts/sos_idt.o \
       io/sos_io.o \
       io/sos_shell.o \
	   commands/sos_cmdfiles.o \
	   commands/sos_cmdgames.o \
	   commands/sos_cmdgui.o \
	   commands/sos_cmdhelp.o \
	   commands/sos_cmdinfo.o \
	   commands/sos_cmdnetwork.o \
	   commands/sos_cmdshell.o \
	   commands/sos_cmdsys.o \
	   commands/sos_cmdtime.o \
       commands/sos_cmds.o \
       drivers/input/sos_keyboard.o \
	   drivers/input/sos_mouse.o \
	   drivers/input/sos_mousecalib.o \
       drivers/video/sos_vga.o \
       drivers/video/sos_vgraphics.o \
	   drivers/e1000/sos_e1000.o \
	   drivers/audio/sos_audio.o \
	   drivers/pci/sos_pci.o \
       libraries/sos_string.o \
       libraries/sos_memory.o \
       libraries/sos_stdio.o \
	   libraries/sos_div64.o \
	   games/sos_snake.o \
	   games/sos_tetris.o \
	   games/sos_pong.o \
	   games/sos_breakout.o \
	   games/sos_minesweeper.o \
	   games/sos_2048.o \
	   games/sos_lifesim.o \
	   games/sos_memorygame.o \
	   games/sos_spaceshooter.o \
	   games/sos_tictactoe.o \
	   games/sos_mirrorgame.o \
	   games/sos_typeracer.o \
	   games/sos_lunarlander.o \
	   games/sos_logiccircuit.o \
	   networking/sos_net.o

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

networking/sos_net.o: include/sos_net.h networking/sos_net.c
	$(CC) $(CFLAGS) -c networking/sos_net.c -o networking/sos_net.o

commands/sos_cmdfiles.o: include/sos_cmdfiles.h commands/sos_cmdfiles.c
	$(CC) $(CFLAGS) -c commands/sos_cmdfiles.c -o commands/sos_cmdfiles.o

commands/sos_cmdgames.o: include/sos_cmdgames.h commands/sos_cmdgames.c
	$(CC) $(CFLAGS) -c commands/sos_cmdgames.c -o commands/sos_cmdgames.o

commands/sos_cmdgui.o: include/sos_cmdgui.h commands/sos_cmdgui.c
	$(CC) $(CFLAGS) -c commands/sos_cmdgui.c -o commands/sos_cmdgui.o

commands/sos_cmdhelp.o: include/sos_cmdhelp.h commands/sos_cmdhelp.c
	$(CC) $(CFLAGS) -c commands/sos_cmdhelp.c -o commands/sos_cmdhelp.o

commands/sos_cmdinfo.o: include/sos_cmdinfo.h commands/sos_cmdinfo.c
	$(CC) $(CFLAGS) -c commands/sos_cmdinfo.c -o commands/sos_cmdinfo.o

commands/sos_cmdnetwork.o: include/sos_cmdnetwork.h commands/sos_cmdnetwork.c
	$(CC) $(CFLAGS) -c commands/sos_cmdnetwork.c -o commands/sos_cmdnetwork.o

commands/sos_cmdshell.o: include/sos_cmdshell.h commands/sos_cmdshell.c
	$(CC) $(CFLAGS) -c commands/sos_cmdshell.c -o commands/sos_cmdshell.o

commands/sos_cmdsys.o: include/sos_cmdsys.h commands/sos_cmdsys.c
	$(CC) $(CFLAGS) -c commands/sos_cmdsys.c -o commands/sos_cmdsys.o

commands/sos_cmdtime.o: include/sos_cmdtime.h commands/sos_cmdtime.c
	$(CC) $(CFLAGS) -c commands/sos_cmdtime.c -o commands/sos_cmdtime.o

commands/sos_cmds.o: include/sos_cmds.h commands/sos_cmds.c
	$(CC) $(CFLAGS) -c commands/sos_cmds.c -o commands/sos_cmds.o

games/sos_snake.o: include/sos_snake.h games/sos_snake.c
	$(CC) $(CFLAGS) -c games/sos_snake.c -o games/sos_snake.o

games/sos_tetris.o: include/sos_tetris.h games/sos_tetris.c
	$(CC) $(CFLAGS) -c games/sos_tetris.c -o games/sos_tetris.o

games/sos_pong.o: include/sos_pong.h games/sos_pong.c
	$(CC) $(CFLAGS) -c games/sos_pong.c -o games/sos_pong.o

games/sos_breakout.o: include/sos_breakout.h games/sos_breakout.c
	$(CC) $(CFLAGS) -c games/sos_breakout.c -o games/sos_breakout.o

games/sos_minesweeper.o: include/sos_minesweeper.h games/sos_minesweeper.c
	$(CC) $(CFLAGS) -c games/sos_minesweeper.c -o games/sos_minesweeper.o

games/sos_logiccircuit.o: include/sos_logiccircuit.h games/sos_logiccircuit.c
	$(CC) $(CFLAGS) -c games/sos_logiccircuit.c -o games/sos_logiccircuit.o

games/sos_2048.o: include/sos_2048.h games/sos_2048.c
	$(CC) $(CFLAGS) -c games/sos_2048.c -o games/sos_2048.o

games/sos_lifesim.o: include/sos_lifesim.h games/sos_lifesim.c
	$(CC) $(CFLAGS) -c games/sos_lifesim.c -o games/sos_lifesim.o

games/sos_memorygame.o: include/sos_memorygame.h games/sos_memorygame.c
	$(CC) $(CFLAGS) -c games/sos_memorygame.c -o games/sos_memorygame.o

games/sos_spaceshooter.o: include/sos_spaceshooter.h games/sos_spaceshooter.c
	$(CC) $(CFLAGS) -c games/sos_spaceshooter.c -o games/sos_spaceshooter.o

games/sos_tictactoe.o: include/sos_tictactoe.h games/sos_tictactoe.c
	$(CC) $(CFLAGS) -c games/sos_tictactoe.c -o games/sos_tictactoe.o

games/sos_mirrorgame.o: include/sos_mirrorgame.h games/sos_mirrorgame.c
	$(CC) $(CFLAGS) -c games/sos_mirrorgame.c -o games/sos_mirrorgame.o

games/sos_lunarlander.o: include/sos_lunarlander.h games/sos_lunarlander.c
	$(CC) $(CFLAGS) -c games/sos_lunarlander.c -o games/sos_lunarlander.o

games/sos_typeracer.o: include/sos_typeracer.h games/sos_typeracer.c
	$(CC) $(CFLAGS) -c games/sos_typeracer.c -o games/sos_typeracer.o

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

applications/sos_musicplayer.o: include/sos_musicplayer.h applications/sos_musicplayer.c
	$(CC) $(CFLAGS) -c applications/sos_musicplayer.c -o applications/sos_musicplayer.o

drivers/input/sos_keyboard.o: include/sos_keyboard.h drivers/input/sos_keyboard.c
	$(CC) $(CFLAGS) -c drivers/input/sos_keyboard.c -o drivers/input/sos_keyboard.o

drivers/input/sos_mouse.o: include/sos_mouse.h drivers/input/sos_mouse.c
	$(CC) $(CFLAGS) -c drivers/input/sos_mouse.c -o drivers/input/sos_mouse.o

drivers/input/sos_mousecalib.o: include/sos_mousecalib.h drivers/input/sos_mousecalib.c
	$(CC) $(CFLAGS) -c drivers/input/sos_mousecalib.c -o drivers/input/sos_mousecalib.o

drivers/timer/sos_pit.o: include/sos_pit.h drivers/timer/sos_pit.c
	$(CC) $(CFLAGS) -c drivers/timer/sos_pit.c -o drivers/timer/sos_pit.o

drivers/audio/sos_audio.o : include/sos_audio.h drivers/audio/sos_audio.c
	$(CC) $(CFLAGS) -c drivers/audio/sos_audio.c -o drivers/audio/sos_audio.o

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

drivers/pci/sos_pci.o : include/sos_pci.h drivers/pci/sos_pci.c
	$(CC) $(CFLAGS) -c drivers/pci/sos_pci.c -o drivers/pci/sos_pci.o

drivers/e1000/sos_e1000.o : include/sos_e1000.h drivers/e1000/sos_e1000.c
	$(CC) $(CFLAGS) -c drivers/e1000/sos_e1000.c -o drivers/e1000/sos_e1000.o

create-img: 
	qemu-img create -f raw disk.img 10M

hexdump-img:
	hexdump -C disk.img 

clean:
	rm -f applications/*.o commands/*.o bootloader/*.o kernel/*.o drivers/*/*.o libraries/*.o io/*.o games/*.o networking/*.o SmolOS.bin 

run: SmolOS.bin
	@if [ ! -f $(DISK_IMG) ]; then \
		echo "$(DISK_IMG) not found. Creating FAT16 disk..."; \
		qemu-img create -f raw $(DISK_IMG) $(DISK_IMG_SIZE)M; \
	fi
	qemu-system-i386 -kernel SmolOS.bin \
	-drive file=disk.img,format=raw,if=ide,index=0 \
	-audiodev sdl,id=snd0 \
	-machine pcspk-audiodev=snd0 \
	-monitor stdio


.PHONY: all clean run 