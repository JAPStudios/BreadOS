BREADOS PROJECT STATUS

The active starter kernel is the modular source in kermal/. The repository
root also contains an older, consolidated prototype and experimental scripts.
Do not combine the two source layouts or treat the root build scripts as the
build system for kermal/.

ACTIVE STARTER
  kermal/kernel.c
  kermal/terminal.c and terminal.h
  kermal/keyboard.c and keyboard.h
  kermal/shell.c and shell.h
  kermal/boot.s
  kermal/linker.ld
  kermal/grub.cfg
  kermal/Makefile

BUILD
  cd kermal
  make
  make run

Build prerequisites: a Linux toolchain with GCC 32-bit freestanding support,
GNU Make, GRUB BIOS utilities (grub-file and grub-mkrescue), xorriso, and
qemu-system-i386. The ISO output is kermal/build/breados.iso.

The current modular starter targets 32-bit x86, legacy BIOS, GRUB Multiboot v1,
VGA text output, and polling PS/2 keyboard input. Its shell currently provides
help, about, clear, and halt.

This is not yet an Android replacement. It has no ARM/ARM64 phone support,
touchscreen, graphical desktop, networking, audio, phone power management,
Android app compatibility, secure app isolation, or phone installation and
recovery flow. Do not claim those features work until implemented and tested.

The root-level consolidated prototype, test utilities, and instructions are
retained as historical/experimental material. Their test notes explicitly
distinguish source-level checks from a real QEMU boot test.
