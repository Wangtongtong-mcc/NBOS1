CC = i686-elf-gcc -m32 -c -g -I. -nostdinc
#AS = ~/software/nasm/bin/nasm -g
AS=nasm -g
#LD = ld -m elf_i386
LD = i686-elf-ld -m elf_i386

OBJCOPY = i686-elf-objcopy


ORIOBJS = \
	entry.s\
	main.c\
	interrupt.c\
	interrupt_handler.s\
	print.c\
	memory.c\
	task.c\
	process.c\
	diskload.c\
	switchktou.s\
	syscall.c\
	file.c\
	str.c\

LINKOBJS = \
	./target/entry.o\
	./target/main.o\
	./target/interrupt.o\
	./target/interrupt_handler.o\
	./target/memory.o\
	./target/task.o\
	./target/syscall.o\
	./target/file.o\
	./target/process.o\
	./target/diskload.o\
	./target/switchktou.o\
	./target/print.o\
	./target/str.o\

TARGET_DIR = ./target

all : $(TARGET_DIR)/bootsect.bin $(TARGET_DIR)/loader.bin $(TARGET_DIR)/kernel.bin $(TARGET_DIR)/shell.bin $(TARGET_DIR)/mypro.bin

$(TARGET_DIR)/bootsect.bin: bootsect.s
	$(AS) -o $(TARGET_DIR)/bootsect.bin bootsect.s

$(TARGET_DIR)/loader.bin: loader.c read_sector.s
	$(CC) -o $(TARGET_DIR)/loader.o loader.c
	$(AS) -f elf32 -o $(TARGET_DIR)/read_sector.o read_sector.s
	$(LD) $(TARGET_DIR)/loader.o $(TARGET_DIR)/read_sector.o -Ttext 0x7e00 -e loader_main -o $(TARGET_DIR)/loader
	$(OBJCOPY) -S -O binary -j .text $(TARGET_DIR)/loader $(TARGET_DIR)/loader.bin

$(TARGET_DIR)/kernel.bin: $(ORIOBJS)
	$(AS) -f elf32 -o $(TARGET_DIR)/entry.o entry.s
	$(AS) -f elf32 -o $(TARGET_DIR)/interrupt_handler.o interrupt_handler.s
	$(AS) -f elf32 -o $(TARGET_DIR)/switchktou.o switchktou.s
	$(CC) -o $(TARGET_DIR)/print.o print.c
	$(CC) -o $(TARGET_DIR)/memory.o memory.c
	$(CC) -o $(TARGET_DIR)/main.o main.c
	$(CC) -o $(TARGET_DIR)/task.o task.c
	$(CC) -o $(TARGET_DIR)/interrupt.o interrupt.c
	$(CC) -o $(TARGET_DIR)/process.o process.c
	$(CC) -o $(TARGET_DIR)/diskload.o diskload.c
	$(CC) -o $(TARGET_DIR)/file.o file.c
	$(CC) -o $(TARGET_DIR)/syscall.o syscall.c
	$(CC) -o $(TARGET_DIR)/str.o str.c
	$(LD) $(LINKOBJS) -Ttext 0xc0010000 -e entry -o $(TARGET_DIR)/kernel.bin

$(TARGET_DIR)/shell.bin: shell.c unistd.c
	$(CC) -o $(TARGET_DIR)/shell.o shell.c
	$(CC) -o $(TARGET_DIR)/unistd.o unistd.c
	$(LD) $(TARGET_DIR)/shell.o $(TARGET_DIR)/unistd.o -Ttext 0x00000000 -e shell_main -o $(TARGET_DIR)/shell.bin

$(TARGET_DIR)/mypro.bin: mypro.c unistd.c
	$(CC) -o $(TARGET_DIR)/mypro.o mypro.c
	$(LD) $(TARGET_DIR)/mypro.o $(TARGET_DIR)/unistd.o -Ttext 0x00000000 -e main -o $(TARGET_DIR)/mypro.bin



