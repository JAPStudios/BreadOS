# BreadOS

BreadOS is being developed toward a broad operating-system replacement, including Android. The current runnable target is a 32-bit x86 BIOS/GRUB kernel prototype. It is not yet a general-purpose desktop OS or a phone-ready Android replacement.

## One authoritative build path

Use the repository-root Makefile. The root build is the only designated integrated build path:

- kernel.c: kernel entry, VGA text console, shell, in-memory filesystem operations, and snapshot persistence.
- boot.s, linker.ld, grub.cfg: Multiboot v1 startup, memory layout, and boot configuration.
- kernel/keyboard.c, kernel/keyboard.h: keyboard input module linked by the root build.
- kernel/pmm.c, kernel/pmm.h: physical page-frame allocator linked by the root build and initialized from the Multiboot memory map.
- heap.c, heap.h: fixed-size kernel heap linked by the root build.
- host_tests.c: host-side regression tests for file operations, paths, snapshots, heap behavior, and the page-frame allocator.

The files kernel/kernel.c, kernel/boot.s, kernel/linker.ld, kernel/grub.cfg, kernel/terminal.*, and kernel/shell.* belong to an older, separate modular-kernel experiment. They are not inputs to the root Makefile. Do not mix that experiment's entry point or build instructions into the integrated prototype. The source directory therefore contains both linked modules and unbuilt experimental files; the root Makefile is the source of truth for what actually builds.

The separate ata.c/.h and exceptions.c files are experimental modules and are not linked into the root build. The integrated prototype currently contains its own disk I/O implementation.

## Build, test, and run

On Linux, install GCC with 32-bit compilation support, GNU Make, GRUB BIOS utilities, xorriso, and QEMU. Run:

    make
    make test
    make run

The generated ISO is starter.iso. disk.img is a dedicated emulator disk used for explicit save/load snapshots. Attach it only to QEMU, never to a physical drive. make clean preserves the disk image.

A successful compile or host test is not proof of a successful boot. Runtime behavior still needs to be exercised in QEMU.

## File commands

The shell implements help, clear, pwd, ls, cd, mkdir, touch, write, cat, mv, cp, rm, echo, save, load, and halt.

Examples:

    mkdir /Documents
    touch /notes.txt
    write /notes.txt Hello
    mv /notes.txt /Documents/
    mv /Documents/notes.txt /
    save

Files and directories currently live in a small in-memory filesystem. save and load explicitly persist or restore a validated snapshot on the emulator disk; this is not yet a general-purpose on-disk filesystem.

## Memory management status

kernel/pmm.c is a physical page-frame allocator: it tracks usable physical memory in 4 KiB pages and supports allocation and release. It is limited to 32-bit physical addresses. It does not enable paging or virtual memory; those are separate components that still need to be implemented and integrated.

## Work required for the replacement goal

Major missing areas include paging and virtual memory, process/thread scheduling, user/kernel privilege separation, system calls, a real filesystem and storage stack, USB and modern input/display drivers, networking and Wi-Fi, audio, a graphical desktop and touch interface, application/runtime compatibility, security updates, installation/recovery, and support for target architectures and actual phone hardware including ARM64. These must be implemented and tested as integrated capabilities before BreadOS can replace desktop operating systems or Android.
