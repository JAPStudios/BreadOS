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
$(BUILD)/gdt.o: gdt.c gdt.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/gdt_load.o: gdt_load.s | $(BUILD)
	$(AS) --32 $< -o $@
$(BUILD)/memory.o: memory.c memory.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/heap.o: heap.c heap.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/fs.o: fs.c fs.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/ata.o: ata.c ata.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/pfs.o: pfs.c pfs.h ata.h fs.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/paging.o: paging.c paging.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/task.o: task.c task.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/syscall.o: syscall.c syscall.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/keyboard.o: kernel/keyboard.c kernel/keyboard.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/kernel.o: kernel.c memory.h heap.h interrupts.h fs.h pfs.h paging.h gdt.h task.h syscall.h kernel/keyboard.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/interrupts.o: interrupts.s | $(BUILD)
	$(AS) --32 $< -o $@
$(BUILD)/interrupts_c.o: interrupts.c interrupts.h task.h syscall.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/kernel.elf: $(BUILD)/boot.o $(BUILD)/gdt.o $(BUILD)/gdt_load.o $(BUILD)/kernel.o $(BUILD)/keyboard.o $(BUILD)/memory.o $(BUILD)/heap.o $(BUILD)/fs.o $(BUILD)/ata.o $(BUILD)/pfs.o $(BUILD)/paging.o $(BUILD)/task.o $(BUILD)/syscall.o $(BUILD)/interrupts.o $(BUILD)/interrupts_c.o linker.ld
	$(LD) $(LDFLAGS) -o $@ $(BUILD)/boot.o $(BUILD)/gdt.o $(BUILD)/gdt_load.o $(BUILD)/interrupts.o $(BUILD)/interrupts_c.o $(BUILD)/memory.o $(BUILD)/heap.o $(BUILD)/fs.o $(BUILD)/ata.o $(BUILD)/pfs.o $(BUILD)/paging.o $(BUILD)/task.o $(BUILD)/syscall.o $(BUILD)/keyboard.o $(BUILD)/kernel.o
	grub-file --is-x86-multiboot $@
$(BUILD)/breados.iso: $(BUILD)/kernel.elf grub.cfg
	mkdir -p $(BUILD)/iso/boot/grub
	cp $(BUILD)/kernel.elf $(BUILD)/iso/boot/kernel.elf
	cp grub.cfg $(BUILD)/iso/boot/grub/grub.cfg
	grub-mkrescue -o $@ $(BUILD)/iso
iso: $(BUILD)/breados.iso
run: iso
	test -f $(BUILD)/breados-disk.img || truncate -s 16M $(BUILD)/breados-disk.img
	qemu-system-i386 -drive file=$(BUILD)/breados-disk.img,format=raw,if=ide,index=0 -cdrom $(BUILD)/breados.iso -serial stdio
test: iso
	bash tests/boot-smoke.sh
clean:
	rm -rf $(BUILD)
