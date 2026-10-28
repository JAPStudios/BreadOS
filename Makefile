CC = gcc
AS = as
LD = ld
CFLAGS = -m32 -std=gnu11 -ffreestanding -fno-builtin -fno-pie -fno-stack-protector -fno-asynchronous-unwind-tables -mno-sse -mno-mmx -msoft-float -Wall -Wextra -Werror -O2
LDFLAGS = -m elf_i386 -T linker.ld -nostdlib
BUILD = build
.PHONY: all iso run test clean
all: iso
$(BUILD):
	mkdir -p $(BUILD)
$(BUILD)/boot.o: boot.s | $(BUILD)
	$(AS) --32 $< -o $@
$(BUILD)/memory.o: memory.c memory.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/fs.o: fs.c fs.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/paging.o: paging.c paging.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/kernel.o: kernel.c memory.h interrupts.h fs.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/interrupts.o: interrupts.s | $(BUILD)
	$(AS) --32 $< -o $@
$(BUILD)/interrupts_c.o: interrupts.c interrupts.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/kernel.elf: $(BUILD)/boot.o $(BUILD)/kernel.o $(BUILD)/memory.o $(BUILD)/fs.o $(BUILD)/paging.o $(BUILD)/interrupts.o $(BUILD)/interrupts_c.o linker.ld
	$(LD) $(LDFLAGS) -o $@ $(BUILD)/boot.o $(BUILD)/interrupts.o $(BUILD)/interrupts_c.o $(BUILD)/memory.o $(BUILD)/fs.o $(BUILD)/paging.o $(BUILD)/kernel.o
	grub-file --is-x86-multiboot $@
$(BUILD)/breados.iso: $(BUILD)/kernel.elf grub.cfg
	mkdir -p $(BUILD)/iso/boot/grub
	cp $(BUILD)/kernel.elf $(BUILD)/iso/boot/kernel.elf
	cp grub.cfg $(BUILD)/iso/boot/grub/grub.cfg
	grub-mkrescue -o $@ $(BUILD)/iso
iso: $(BUILD)/breados.iso
run: iso
	qemu-system-i386 -cdrom $(BUILD)/breados.iso -serial stdio
test: iso
	bash tests/boot-smoke.sh
clean:
	rm -rf $(BUILD)
