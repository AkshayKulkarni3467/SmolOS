CC = gcc
CFLAGS = -nostdlib -fno-builtin -fno-stack-protector -O2 -Wall -Wextra -m32

OBJS = boot.o kernel.o

all: smol.bin

smol.bin: $(OBJS)
	$(CC) -m32 -nostdlib -o smol.bin $(OBJS) -T linker.ld

boot.o: boot.s
	$(CC) $(CFLAGS) -c boot.s -o boot.o

kernel.o: kernel.c
	$(CC) $(CFLAGS) -c kernel.c -o kernel.o

clean:
	rm -f *.o smol.bin

run: smol.bin
	qemu-system-i386 -kernel smol.bin

.PHONY: all clean run