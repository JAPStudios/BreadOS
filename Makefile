CC = gcc
AS = as
LD = ld
HOSTCC ?= cc
USER_CFLAGS = -m32 -std=c11 -ffreestanding -fno-pie -fno-pic -fno-stack-protector -fno-asynchronous-unwind-tables -mno-sse -mno-mmx -msoft-float -Wall -Wextra -Werror -O2
CFLAGS = -m32 -std=gnu11 -ffreestanding -fno-builtin -fno-pie -fno-stack-protector -fno-asynchronous-unwind-tables -mno-sse -mno-mmx -msoft-float -Wall -Wextra -Werror -O2
LDFLAGS = -m elf_i386 -T linker.ld -nostdlib
BUILD = build
.PHONY: all iso user-apps run test test-sanitize clean
all: iso user-apps
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
$(BUILD)/pci.o: pci.c pci.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/audio.o: audio.c audio.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/heap.o: heap.c heap.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/fs.o: fs.c fs.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/ata.o: ata.c ata.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/pfs.o: pfs.c pfs.h ata.h fs.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/vfs.o: vfs.c vfs.h fs.h pfs.h console.h memory.h interrupts.h process.h task.h kernel/keyboard.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/paging.o: paging.c paging.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/elf_loader.o: elf_loader.c elf_loader.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/process.o: process.c process.h task.h memory.h paging.h syscall.h elf_loader.h vfs.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/syscall.o: syscall.c syscall.h task.h console.h paging.h ipc.h vfs.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/video.o: video.c video.h paging.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/theme.o: theme.c theme.h video.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/menu.o: menu.c menu.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/notification.o: notification.c notification.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/clipboard.o: clipboard.c clipboard.h secure_memory.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/reminder.o: reminder.c reminder.h notification.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/idle.o: idle.c idle.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/launcher.o: launcher.c launcher.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/privacy.o: privacy.c privacy.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/secure_memory.o: secure_memory.c secure_memory.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/desktop.o: desktop.c desktop.h video.h theme.h memory.h task.h kernel/keyboard.h kernel/mouse.h notification.h clipboard.h reminder.h idle.h launcher.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/task.o: task.c task.h paging.h gdt.h ipc.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/ipc.o: ipc.c ipc.h task.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/keyboard.o: kernel/keyboard.c kernel/keyboard.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/mouse.o: kernel/mouse.c kernel/mouse.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/kernel.o: kernel.c audio.h memory.h privacy.h heap.h interrupts.h fs.h pfs.h vfs.h elf_loader.h paging.h video.h gdt.h task.h kernel/keyboard.h kernel/mouse.h process.h console.h theme.h menu.h desktop.h notification.h clipboard.h reminder.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/interrupts.o: interrupts.s | $(BUILD)
	$(AS) --32 $< -o $@
$(BUILD)/interrupts_c.o: interrupts.c interrupts.h task.h video.h kernel/keyboard.h kernel/mouse.h syscall.h process.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/kernel.elf: $(BUILD)/elf_loader.o $(BUILD)/pci.o $(BUILD)/audio.o $(BUILD)/boot.o $(BUILD)/gdt.o $(BUILD)/gdt_load.o $(BUILD)/usermode.o $(BUILD)/process.o $(BUILD)/syscall.o $(BUILD)/kernel.o $(BUILD)/keyboard.o $(BUILD)/mouse.o $(BUILD)/memory.o $(BUILD)/heap.o $(BUILD)/fs.o $(BUILD)/ata.o $(BUILD)/pfs.o $(BUILD)/vfs.o $(BUILD)/paging.o $(BUILD)/video.o $(BUILD)/theme.o $(BUILD)/menu.o $(BUILD)/desktop.o $(BUILD)/notification.o $(BUILD)/clipboard.o $(BUILD)/reminder.o $(BUILD)/idle.o $(BUILD)/launcher.o $(BUILD)/privacy.o $(BUILD)/secure_memory.o $(BUILD)/task.o $(BUILD)/ipc.o $(BUILD)/interrupts.o $(BUILD)/interrupts_c.o linker.ld
	$(LD) $(LDFLAGS) -o $@ $(BUILD)/elf_loader.o $(BUILD)/pci.o $(BUILD)/audio.o $(BUILD)/boot.o $(BUILD)/gdt.o $(BUILD)/gdt_load.o $(BUILD)/usermode.o $(BUILD)/process.o $(BUILD)/syscall.o $(BUILD)/interrupts.o $(BUILD)/interrupts_c.o $(BUILD)/memory.o $(BUILD)/heap.o $(BUILD)/fs.o $(BUILD)/ata.o $(BUILD)/pfs.o $(BUILD)/vfs.o $(BUILD)/paging.o $(BUILD)/video.o $(BUILD)/theme.o $(BUILD)/menu.o $(BUILD)/desktop.o $(BUILD)/notification.o $(BUILD)/clipboard.o $(BUILD)/reminder.o $(BUILD)/idle.o $(BUILD)/launcher.o $(BUILD)/privacy.o $(BUILD)/secure_memory.o $(BUILD)/task.o $(BUILD)/ipc.o $(BUILD)/keyboard.o $(BUILD)/mouse.o $(BUILD)/kernel.o
	grub-file --is-x86-multiboot $@
$(BUILD)/breados.iso: $(BUILD)/kernel.elf grub.cfg
	mkdir -p $(BUILD)/iso/boot/grub
	cp $(BUILD)/kernel.elf $(BUILD)/iso/boot/kernel.elf
	cp grub.cfg $(BUILD)/iso/boot/grub/grub.cfg
	grub-mkrescue -o $@ $(BUILD)/iso
$(BUILD)/user/hello.elf: user/examples/hello.c user/include/breados/syscall.h user/linker.ld | $(BUILD)
	mkdir -p $(BUILD)/user
	$(CC) $(USER_CFLAGS) -Iuser/include -c user/examples/hello.c -o $(BUILD)/user/hello.o
	$(LD) -m elf_i386 -z max-page-size=0x1000 -T user/linker.ld -nostdlib -o $@ $(BUILD)/user/hello.o
$(BUILD)/user/guard.elf: user/examples/guard.c user/include/breados/syscall.h user/linker.ld | $(BUILD)
	mkdir -p $(BUILD)/user
	$(CC) $(USER_CFLAGS) -Iuser/include -c user/examples/guard.c -o $(BUILD)/user/guard.o
	$(LD) -m elf_i386 -z max-page-size=0x1000 -T user/linker.ld -nostdlib -o $@ $(BUILD)/user/guard.o
user-apps: $(BUILD)/user/hello.elf $(BUILD)/user/guard.elf

$(BUILD)/user-app-elf-host-test: tests/user-app-elf-host.c elf_loader.c elf_loader.h | $(BUILD)
	$(HOSTCC) -std=c11 -Wall -Wextra -Werror -O2 -I. tests/user-app-elf-host.c elf_loader.c -o $@

$(BUILD)/pci-host-test: tests/pci-host.c pci.c pci.h | $(BUILD)
	$(HOSTCC) -std=c11 -Wall -Wextra -Werror -O2 -I. tests/pci-host.c pci.c -o $@
$(BUILD)/audio-host-test: tests/audio-host.c audio.c audio.h | $(BUILD)
	$(HOSTCC) -std=c11 -Wall -Wextra -Werror -O2 -I. tests/audio-host.c audio.c -o $@
$(BUILD)/heap-host-test: tests/heap-host.c heap.c heap.h | $(BUILD)
	$(HOSTCC) -std=c11 -Wall -Wextra -Werror -O2 -I. tests/heap-host.c heap.c -o $@
$(BUILD)/filesystem-host-test: tests/filesystem-host.c fs.c pfs.c fs.h pfs.h ata.h | $(BUILD)
	$(HOSTCC) -std=c11 -Wall -Wextra -Werror -O2 -I. tests/filesystem-host.c fs.c pfs.c -o $@
$(BUILD)/keyboard-host-test: tests/keyboard-host.c kernel/keyboard.c kernel/keyboard.h | $(BUILD)
	$(HOSTCC) -std=c11 -Wall -Wextra -Werror -O2 -I. tests/keyboard-host.c kernel/keyboard.c -o $@
$(BUILD)/task-host-test: tests/task-host.c task.c task.h paging.h ipc.h | $(BUILD)
	$(HOSTCC) -std=c11 -Wall -Wextra -Werror -O2 -I. tests/task-host.c task.c -o $@
$(BUILD)/vfs-host-test: tests/vfs-host.c vfs.c fs.c vfs.h fs.h | $(BUILD)
	$(HOSTCC) -std=c11 -Wall -Wextra -Werror -O2 -I. tests/vfs-host.c vfs.c fs.c -o $@
$(BUILD)/ipc-host-test: tests/ipc-host.c ipc.c ipc.h task.h | $(BUILD)
	$(HOSTCC) -std=c11 -Wall -Wextra -Werror -O2 -I. tests/ipc-host.c ipc.c -o $@
$(BUILD)/mouse-host-test: tests/mouse-host.c kernel/mouse.c kernel/mouse.h | $(BUILD)
	$(HOSTCC) -std=c11 -Wall -Wextra -Werror -O2 -I. tests/mouse-host.c kernel/mouse.c -o $@
$(BUILD)/theme-host-test: tests/theme-host.c theme.c theme.h video.h | $(BUILD)
	$(HOSTCC) -std=c11 -Wall -Wextra -Werror -O2 -I. tests/theme-host.c theme.c -o $@
$(BUILD)/menu-host-test: tests/menu-host.c menu.c menu.h | $(BUILD)
	$(HOSTCC) -std=c11 -Wall -Wextra -Werror -O2 -I. tests/menu-host.c menu.c -o $@
$(BUILD)/notification-host-test: tests/notification-host.c notification.c notification.h | $(BUILD)
	$(HOSTCC) -std=c11 -Wall -Wextra -Werror -O2 -I. tests/notification-host.c notification.c -o $@
$(BUILD)/clipboard-host-test: tests/clipboard-host.c clipboard.c clipboard.h secure_memory.c secure_memory.h | $(BUILD)
	$(HOSTCC) -std=c11 -Wall -Wextra -Werror -O2 -I. tests/clipboard-host.c clipboard.c secure_memory.c -o $@
$(BUILD)/reminder-host-test: tests/reminder-host.c reminder.c reminder.h notification.c notification.h | $(BUILD)
	$(HOSTCC) -std=c11 -Wall -Wextra -Werror -O2 -I. tests/reminder-host.c reminder.c notification.c -o $@
$(BUILD)/idle-host-test: tests/idle-host.c idle.c idle.h | $(BUILD)
	$(HOSTCC) -std=c11 -Wall -Wextra -Werror -O2 -I. tests/idle-host.c idle.c -o $@
$(BUILD)/launcher-host-test: tests/launcher-host.c launcher.c launcher.h | $(BUILD)
	$(HOSTCC) -std=c11 -Wall -Wextra -Werror -O2 -I. tests/launcher-host.c launcher.c -o $@
$(BUILD)/privacy-host-test: tests/privacy-host.c privacy.c privacy.h | $(BUILD)
	$(HOSTCC) -std=c11 -Wall -Wextra -Werror -O2 -I. tests/privacy-host.c privacy.c -o $@
$(BUILD)/elf-loader-host-test: tests/elf-loader-host.c elf_loader.c elf_loader.h | $(BUILD)
	$(HOSTCC) -std=c11 -Wall -Wextra -Werror -O2 -I. tests/elf-loader-host.c elf_loader.c -o $@
$(BUILD)/secure-memory-host-test: tests/secure-memory-host.c secure_memory.c secure_memory.h | $(BUILD)
	$(HOSTCC) -std=c11 -Wall -Wextra -Werror -O2 -I. tests/secure-memory-host.c secure_memory.c -o $@
$(BUILD)/video-host-test: tests/video-host.c video.c video.h paging.h | $(BUILD)
	$(HOSTCC) -std=c11 -Wall -Wextra -Werror -O2 -I. tests/video-host.c video.c -o $@

$(BUILD)/syscall-host-test: tests/syscall-host.c syscall.c syscall.h task.h paging.h ipc.h console.h vfs.h | $(BUILD)
	$(HOSTCC) -std=c11 -Wall -Wextra -Werror -O2 -I. tests/syscall-host.c syscall.c -o $@

$(BUILD)/ipc-host-sanitize-test: tests/ipc-host.c ipc.c ipc.h task.h | $(BUILD)
	$(HOSTCC) -std=c11 -Wall -Wextra -Werror -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -I. tests/ipc-host.c ipc.c -o $@
$(BUILD)/syscall-host-sanitize-test: tests/syscall-host.c syscall.c syscall.h task.h paging.h ipc.h console.h vfs.h | $(BUILD)
	$(HOSTCC) -std=c11 -Wall -Wextra -Werror -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -I. tests/syscall-host.c syscall.c -o $@
$(BUILD)/vfs-host-sanitize-test: tests/vfs-host.c vfs.c fs.c vfs.h fs.h | $(BUILD)
	$(HOSTCC) -std=c11 -Wall -Wextra -Werror -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -I. tests/vfs-host.c vfs.c fs.c -o $@
$(BUILD)/elf-loader-host-sanitize-test: tests/elf-loader-host.c elf_loader.c elf_loader.h | $(BUILD)
	$(HOSTCC) -std=c11 -Wall -Wextra -Werror -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -I. tests/elf-loader-host.c elf_loader.c -o $@
$(BUILD)/filesystem-host-sanitize-test: tests/filesystem-host.c fs.c pfs.c fs.h pfs.h ata.h | $(BUILD)
	$(HOSTCC) -std=c11 -Wall -Wextra -Werror -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -I. tests/filesystem-host.c fs.c pfs.c -o $@
test-sanitize: $(BUILD)/syscall-host-sanitize-test $(BUILD)/ipc-host-sanitize-test $(BUILD)/vfs-host-sanitize-test $(BUILD)/elf-loader-host-sanitize-test $(BUILD)/filesystem-host-sanitize-test
	ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 $(BUILD)/syscall-host-sanitize-test
	ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 $(BUILD)/ipc-host-sanitize-test
	ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 $(BUILD)/vfs-host-sanitize-test
	ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 $(BUILD)/elf-loader-host-sanitize-test
	ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 $(BUILD)/filesystem-host-sanitize-test

$(BUILD)/mk-breadfs-image: tools/mk-breadfs-image.c pfs.c pfs.h ata.h fs.h | $(BUILD)
	$(HOSTCC) -std=c11 -Wall -Wextra -Werror -O2 -I. tools/mk-breadfs-image.c pfs.c -o $@

iso: $(BUILD)/breados.iso
run: iso user-apps $(BUILD)/mk-breadfs-image
	test -f $(BUILD)/breados-disk.img || $(BUILD)/mk-breadfs-image $(BUILD)/breados-disk.img $(BUILD)/user/hello.elf
	qemu-system-i386 -drive file=$(BUILD)/breados-disk.img,format=raw,if=ide,index=0 -cdrom $(BUILD)/breados.iso -serial stdio
test: iso user-apps $(BUILD)/mk-breadfs-image $(BUILD)/user-app-elf-host-test $(BUILD)/elf-loader-host-test $(BUILD)/pci-host-test $(BUILD)/audio-host-test $(BUILD)/heap-host-test $(BUILD)/filesystem-host-test $(BUILD)/keyboard-host-test $(BUILD)/task-host-test $(BUILD)/ipc-host-test $(BUILD)/vfs-host-test $(BUILD)/video-host-test $(BUILD)/mouse-host-test $(BUILD)/theme-host-test $(BUILD)/menu-host-test $(BUILD)/notification-host-test $(BUILD)/clipboard-host-test $(BUILD)/reminder-host-test $(BUILD)/idle-host-test $(BUILD)/launcher-host-test $(BUILD)/privacy-host-test $(BUILD)/secure-memory-host-test $(BUILD)/syscall-host-test
	$(BUILD)/user-app-elf-host-test $(BUILD)/user/hello.elf
	$(BUILD)/user-app-elf-host-test $(BUILD)/user/guard.elf
	$(BUILD)/elf-loader-host-test
	$(BUILD)/pci-host-test
	$(BUILD)/audio-host-test
	$(BUILD)/heap-host-test
	$(BUILD)/filesystem-host-test
	$(BUILD)/keyboard-host-test
	$(BUILD)/task-host-test
	$(BUILD)/ipc-host-test
	$(BUILD)/vfs-host-test
	$(BUILD)/video-host-test
	$(BUILD)/mouse-host-test
	$(BUILD)/theme-host-test
	$(BUILD)/menu-host-test
	$(BUILD)/notification-host-test
	$(BUILD)/clipboard-host-test
	$(BUILD)/reminder-host-test
	$(BUILD)/idle-host-test
	$(BUILD)/launcher-host-test
	$(BUILD)/privacy-host-test
	$(BUILD)/secure-memory-host-test
	$(BUILD)/syscall-host-test
	bash tests/boot-smoke.sh
clean:
	rm -rf $(BUILD)
