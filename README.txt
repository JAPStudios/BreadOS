BREADOS — SOURCE OF TRUTH

Use README.md for the current project status, authoritative build path, file
layout, commands, and remaining work.

The authoritative build path is the repository-root Makefile:
  make
  make test
  make run

The older files under kernel/kernel.c, kernel/boot.s, kernel/linker.ld,
kernel/grub.cfg, kernel/terminal.*, and kernel/shell.* are a separate,
unbuilt modular-kernel experiment. Do not treat them as part of the root
integrated kernel. The kernel/keyboard.* and kernel/pmm.* modules are linked
by the root Makefile.

Only attach disk.img to QEMU. Never attach this experimental disk image to a
physical drive.
