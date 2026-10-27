BREADOS — SOURCE OF TRUTH

The active build and source layout are documented in README.md.

The build files stay at the repository root:
  Makefile
  boot.s
  kernel.c
  linker.ld
  grub.cfg

Reusable kernel modules are under kernel/:
  keyboard.c / keyboard.h
  pmm.c / pmm.h

Build and test from the repository root:
  make
  make test
  make run

Only attach disk.img to QEMU. Never attach it to a physical drive.
