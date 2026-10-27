# BreadOS

BreadOS is intended to grow into an operating system that can replace existing operating systems, including Android. The repository is not at that capability level yet. Its current runnable target is a **32-bit x86 BIOS/GRUB kernel prototype**; it is not currently a general-purpose desktop OS or a phone-ready Android replacement.

## Active integrated prototype

The repository-root source is the integrated prototype and the primary development path:

- `kernel.c`: kernel entry, VGA text console, PS/2 keyboard polling, shell, in-memory file operations, and snapshot persistence to an attached ATA disk.
- `boot.s`, `linker.ld`, `grub.cfg`: Multiboot v1 startup and image layout.
- `ata.c/.h`: separate ATA PIO driver module retained for development; the integrated `kernel.c` currently has its own disk I/O implementation.
- `heap.c/.h`, `exceptions.c`: separate experimental modules, not currently linked into the integrated kernel.
- `host_tests.c`: host-side regression tests for filesystem operations, path parsing, and snapshot validation.
- `kernel/`: a separate minimal modular kernel experiment. It is not the integrated prototype's build path.

## Build, test, and run

On Linux, install GCC with 32-bit compilation support, GNU Make, GRUB BIOS utilities, xorriso, and QEMU. Then:

```sh
make
make test
make run
```

The ISO is `starter.iso`. The emulator's persistent test disk is `disk.img`; `make clean` deliberately preserves it. **Only attach this disk image to QEMU. Do not point this experimental kernel at a real drive.**

A successful compile or ISO creation does not prove the kernel boots or that all commands and persistence work. Run-time behavior needs QEMU testing.

## What works in the current design

The integrated prototype is designed to provide a text shell with `help`, `clear`, `pwd`, `ls`, `cd`, `mkdir`, `touch`, `write`, `cat`, `mv`, `cp`, `rm`, `echo`, `save`, `load`, and `halt`. Files are held in RAM; `save` and `load` explicitly store or restore a validated snapshot on the attached ATA disk.

These are implemented code paths, not a claim that every command has passed a boot test. The build and host regression checks are intended to expose failures.

## Work still required for the replacement goal

A broad OS replacement requires substantially more than a shell and a small in-memory filesystem. Major missing areas include protected-mode memory management and paging, process/thread scheduling, user/kernel privilege separation, system calls, a real filesystem and storage stack, USB and modern input/display drivers, networking and Wi-Fi, audio, a graphical desktop and touch interface, application/runtime compatibility, security updates, installation/recovery, and support for target architectures and actual phone hardware (including ARM64). These need to be implemented and tested as integrated capabilities before BreadOS can replace Android or desktop operating systems.

The current kernel targets legacy BIOS/GRUB Multiboot v1 and 32-bit x86. It cannot boot as a native Android replacement on an ARM64 phone.
