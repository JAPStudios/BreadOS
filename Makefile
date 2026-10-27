CC ?= gcc
QEMU ?= qemu-system-i386
GRUB_FILE ?= grub-file
GRUB_MKRESCUE ?= grub-mkrescue
PYTHON ?= python3

BUILD := build
ISO_ROOT := $(BUILD)/iso
KERNEL := $(BUILD)/kernel.elf
ISO := starter.iso
OBJECTS := $(BUILD)/boot.o $(BUILD)/kernel.o

CFLAGS := -m32 -std=gnu11 -march=i686 -ffreestanding -fno-builtin -fno-pie \
          -fno-stack-protector -fno-asynchronous-unwind-tables \
          -mno-sse -mno-mmx -msoft-float -Wall -Wextra -O2
LDFLAGS := -m32 -nostdlib -static -no-pie -Wl,--build-id=none -Wl,-T,linker.ld

.PHONY: all kernel iso run clean check-tools check-qemu host-test test

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

$(BUILD)/kernel.o: kernel.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(KERNEL): $(OBJECTS) linker.ld
	$(CC) $(LDFLAGS) -o $@ $(OBJECTS)
	$(GRUB_FILE) --is-x86-multiboot $@

kernel: $(KERNEL)

iso: check-tools $(ISO)

$(ISO): $(KERNEL) grub.cfg
	mkdir -p $(ISO_ROOT)/boot/grub
	cp $(KERNEL) $(ISO_ROOT)/boot/kernel.elf
	cp grub.cfg $(ISO_ROOT)/boot/grub/grub.cfg
	$(GRUB_MKRESCUE) -o $@ $(ISO_ROOT)

check-qemu:
	@command -v $(QEMU) >/dev/null || { echo "Missing QEMU"; exit 1; }

disk.img:
	$(PYTHON) create_disk.py

run: check-qemu iso disk.img
	$(QEMU) -m 64 -boot d -cdrom $(ISO) -drive file=disk.img,format=raw,if=ide,index=0,media=disk

host-test: | $(BUILD)
	$(CC) -std=gnu11 -fno-builtin -Wall -Wextra -O2 host_tests.c -o $(BUILD)/host_tests
	$(BUILD)/host_tests

test: host-test

clean:
	rm -rf $(BUILD) $(ISO)
