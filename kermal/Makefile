CC ?= gcc
QEMU ?= qemu-system-i386
GRUB_FILE ?= grub-file
GRUB_MKRESCUE ?= grub-mkrescue

BUILD := build
ISO_ROOT := $(BUILD)/iso
OBJECTS := $(BUILD)/boot.o $(BUILD)/kernel.o $(BUILD)/terminal.o $(BUILD)/keyboard.o $(BUILD)/shell.o
KERNEL := $(BUILD)/kernel.elf
ISO := $(BUILD)/breados.iso

CFLAGS := -m32 -std=gnu11 -ffreestanding -fno-builtin -fno-pie \
          -fno-stack-protector -fno-asynchronous-unwind-tables \
          -mno-sse -mno-mmx -Wall -Wextra -O2
LDFLAGS := -m32 -nostdlib -no-pie -Wl,--build-id=none -Wl,-T,linker.ld

.PHONY: all iso run clean check-tools check-qemu

all: iso

check-tools:
	@command -v $(CC) >/dev/null || { echo "Missing compiler: $(CC)"; exit 1; }
	@command -v $(GRUB_FILE) >/dev/null || { echo "Missing grub-file"; exit 1; }
	@command -v $(GRUB_MKRESCUE) >/dev/null || { echo "Missing grub-mkrescue"; exit 1; }
	@command -v xorriso >/dev/null || { echo "Missing xorriso"; exit 1; }

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/boot.o: boot.s | $(BUILD)
	$(CC) -m32 -c $< -o $@

$(BUILD)/kernel.o: kernel.c terminal.h shell.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/terminal.o: terminal.c terminal.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/keyboard.o: keyboard.c keyboard.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/shell.o: shell.c shell.h keyboard.h terminal.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(KERNEL): $(OBJECTS) linker.ld
	$(CC) $(LDFLAGS) -o $@ $(OBJECTS)
	$(GRUB_FILE) --is-x86-multiboot $@

iso: check-tools $(ISO)

$(ISO): $(KERNEL) grub.cfg
	mkdir -p $(ISO_ROOT)/boot/grub
	cp $(KERNEL) $(ISO_ROOT)/boot/kernel.elf
	cp grub.cfg $(ISO_ROOT)/boot/grub/grub.cfg
	$(GRUB_MKRESCUE) -o $@ $(ISO_ROOT)

check-qemu:
	@command -v $(QEMU) >/dev/null || { echo "Missing QEMU: $(QEMU)"; exit 1; }

run: check-tools check-qemu $(ISO)
	$(QEMU) -cdrom $(ISO) -boot d

clean:
	rm -rf $(BUILD)
