CC = gcc
AS = as
LD = ld
HOSTCC ?= cc
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
$(BUILD)/usermode.o: usermode.s | $(BUILD)
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
$(BUILD)/process.o: process.c process.h memory.h paging.h syscall.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/syscall.o: syscall.c syscall.h video.h console.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/video.o: video.c video.h paging.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/theme.o: theme.c theme.h video.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/menu.o: menu.c menu.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/desktop.o: desktop.c desktop.h video.h theme.h memory.h task.h kernel/keyboard.h kernel/mouse.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/task.o: task.c task.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/keyboard.o: kernel/keyboard.c kernel/keyboard.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/mouse.o: kernel/mouse.c kernel/mouse.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/kernel.o: kernel.c memory.h heap.h interrupts.h fs.h pfs.h paging.h video.h gdt.h task.h kernel/keyboard.h kernel/mouse.h process.h console.h theme.h menu.h desktop.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/interrupts.o: interrupts.s | $(BUILD)
	$(AS) --32 $< -o $@
$(BUILD)/interrupts_c.o: interrupts.c interrupts.h task.h video.h kernel/keyboard.h kernel/mouse.h syscall.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/kernel.elf: $(BUILD)/boot.o $(BUILD)/gdt.o $(BUILD)/gdt_load.o $(BUILD)/usermode.o $(BUILD)/process.o $(BUILD)/syscall.o $(BUILD)/kernel.o $(BUILD)/keyboard.o $(BUILD)/mouse.o $(BUILD)/memory.o $(BUILD)/heap.o $(BUILD)/fs.o $(BUILD)/ata.o $(BUILD)/pfs.o $(BUILD)/paging.o $(BUILD)/video.o $(BUILD)/theme.o $(BUILD)/menu.o $(BUILD)/desktop.o $(BUILD)/task.o $(BUILD)/interrupts.o $(BUILD)/interrupts_c.o linker.ld
	$(LD) $(LDFLAGS) -o $@ $(BUILD)/boot.o $(BUILD)/gdt.o $(BUILD)/gdt_load.o $(BUILD)/usermode.o $(BUILD)/process.o $(BUILD)/syscall.o $(BUILD)/interrupts.o $(BUILD)/interrupts_c.o $(BUILD)/memory.o $(BUILD)/heap.o $(BUILD)/fs.o $(BUILD)/ata.o $(BUILD)/pfs.o $(BUILD)/paging.o $(BUILD)/video.o $(BUILD)/theme.o $(BUILD)/menu.o $(BUILD)/desktop.o $(BUILD)/task.o $(BUILD)/keyboard.o $(BUILD)/mouse.o $(BUILD)/kernel.o
	grub-file --is-x86-multiboot $@
$(BUILD)/breados.iso: $(BUILD)/kernel.elf grub.cfg
	mkdir -p $(BUILD)/iso/boot/grub
	cp $(BUILD)/kernel.elf $(BUILD)/iso/boot/kernel.elf
	cp grub.cfg $(BUILD)/iso/boot/grub/grub.cfg
	grub-mkrescue -o $@ $(BUILD)/iso
$(BUILD)/filesystem-host-test: tests/filesystem-host.c fs.c pfs.c fs.h pfs.h ata.h | $(BUILD)
	$(HOSTCC) -std=c11 -Wall -Wextra -Werror -O2 -I. tests/filesystem-host.c fs.c pfs.c -o $@
$(BUILD)/keyboard-host-test: tests/keyboard-host.c kernel/keyboard.c kernel/keyboard.h | $(BUILD)
	$(HOSTCC) -std=c11 -Wall -Wextra -Werror -O2 -I. tests/keyboard-host.c kernel/keyboard.c -o $@
$(BUILD)/task-host-test: tests/task-host.c task.c task.h | $(BUILD)
	$(HOSTCC) -std=c11 -Wall -Wextra -Werror -O2 -I. tests/task-host.c task.c -o $@
$(BUILD)/mouse-host-test: tests/mouse-host.c kernel/mouse.c kernel/mouse.h | $(BUILD)
	$(HOSTCC) -std=c11 -Wall -Wextra -Werror -O2 -I. tests/mouse-host.c kernel/mouse.c -o $@
$(BUILD)/theme-host-test: tests/theme-host.c theme.c theme.h video.h | $(BUILD)
	$(HOSTCC) -std=c11 -Wall -Wextra -Werror -O2 -I. tests/theme-host.c theme.c -o $@
$(BUILD)/menu-host-test: tests/menu-host.c menu.c menu.h | $(BUILD)
	$(HOSTCC) -std=c11 -Wall -Wextra -Werror -O2 -I. tests/menu-host.c menu.c -o $@
$(BUILD)/video-host-test: tests/video-host.c video.c video.h paging.h | $(BUILD)
	$(HOSTCC) -std=c11 -Wall -Wextra -Werror -O2 -I. tests/video-host.c video.c -o $@
iso: $(BUILD)/breados.iso
run: iso
	test -f $(BUILD)/breados-disk.img || truncate -s 16M $(BUILD)/breados-disk.img
	qemu-system-i386 -drive file=$(BUILD)/breados-disk.img,format=raw,if=ide,index=0 -cdrom $(BUILD)/breados.iso -serial stdio
test: iso $(BUILD)/filesystem-host-test $(BUILD)/keyboard-host-test $(BUILD)/task-host-test $(BUILD)/video-host-test $(BUILD)/mouse-host-test $(BUILD)/theme-host-test $(BUILD)/menu-host-test
	$(BUILD)/filesystem-host-test
	$(BUILD)/keyboard-host-test
	$(BUILD)/task-host-test
	$(BUILD)/video-host-test
	$(BUILD)/mouse-host-test
	$(BUILD)/theme-host-test
	$(BUILD)/menu-host-test
	bash tests/boot-smoke.sh
clean:
	rm -rf $(BUILD)
