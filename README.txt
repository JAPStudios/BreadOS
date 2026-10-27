BREADOS STATUS

Goal: develop BreadOS into an operating system capable of replacing existing
operating systems, including Android. Current status: early 32-bit x86
BIOS/GRUB kernel prototype, not yet a usable general-purpose or phone OS.

PRIMARY BUILD PATH (repository root)
  kernel.c
  boot.s
  linker.ld
  grub.cfg
  Makefile
  host_tests.c

Build/test/run on Linux with 32-bit GCC support, GNU Make, GRUB BIOS tools,
xorriso, and QEMU:
  make
  make test
  make run

The generated ISO is starter.iso. disk.img is a dedicated emulator disk used
for the prototype's explicit save/load snapshot commands. Never attach a real
disk to this experimental kernel. make clean preserves disk.img.

The integrated prototype includes a text shell, an in-memory filesystem, and
explicit ATA snapshot persistence in kernel.c. Host tests cover filesystem
operations, quoted path parsing, and snapshot validation. These checks do not
replace a real QEMU boot and command-level test.

kermal/ is a separate minimal modular kernel experiment, not the integrated
prototype's build path. heap.c/.h, ata.c/.h, and exceptions.c are separate
experimental modules not currently linked into the integrated kernel.

Major work remains before the replacement goal: paging and memory protection,
processes and scheduling, privilege separation, syscalls, a production-grade
filesystem and driver stack, USB, networking/Wi-Fi, audio, graphics/touch,
application compatibility, security/update infrastructure, installation and
recovery, and target-specific support including ARM64 phone hardware.
