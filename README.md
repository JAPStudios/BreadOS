# BreadOS

BreadOS is an experimental, freestanding 32-bit x86 operating-system kernel. It is a development prototype, not yet a replacement for Windows, macOS, Linux, or Android.

## Current implementation

- Multiboot v1 boot through GRUB, with a VGA text console and a polling PS/2 keyboard shell.
- US keyboard input with Shift and Caps Lock; extended navigation keys are not implemented.
- GDT setup, a TSS, IDT exception stubs, a remapped PIC, and a 100 Hz PIT tick counter.
- A physical frame bitmap initialized from the Multiboot memory map. Allocation is intentionally restricted to the first 16 MiB because that is the only region currently identity-mapped.
- A fixed 256 KiB kernel heap.
- Bootstrap paging with a writable, supervisor-only identity map of the first 16 MiB. This is not process isolation or a complete virtual-memory subsystem.
- A small RAM filesystem and a simple fixed-sector BreadFS v1 filesystem over primary-master ATA PIO. Both support directory paths and file moves, but remain limited to 16 total file/directory entries, 15 characters per path, and 255 bytes per file; BreadFS is not a general-purpose filesystem.
- Serial console output for boot diagnostics and a QEMU smoke test.

The shell currently provides `help`, `about`, `memory`, `storage`, `uptime`, `tasks`, `ls`, `mkdir PATH`, `rmdir PATH`, `cat PATH`, `write PATH TEXT`, `mv SOURCE DEST`, `rm PATH`, `clear`, `reboot`, and `halt`. Paths are relative to the root (there is no current-directory command yet). Example: `mkdir docs`, `write docs/note.txt hello`, `mv docs/note.txt note.txt`, `cat note.txt`, `rmdir docs`.

## Build requirements

On a Debian/Ubuntu Linux host, install the build and emulator dependencies:

```sh
sudo apt-get update
sudo apt-get install build-essential binutils gcc-multilib grub-pc-bin grub-common xorriso mtools qemu-system-x86
```

Build and run the checks:

```sh
make clean iso
make test
make run
```

The build produces `build/kernel.elf` and `build/breados.iso`. `make test` runs host-side filesystem and keyboard regression tests, then boots the ISO in QEMU and checks serial startup output and persistence across two boots. These are smoke/regression tests, not proof of full hardware compatibility or complete OS behavior.

## Disk safety

`make run` uses `build/breados-disk.img`. The smoke test recreates this file as a zero-filled 16 MiB image. Use a disposable emulator image only; never point the emulator at a physical drive or an image containing data that must be preserved.

BreadFS initializes a disk only when its first sector is blank. If the first sector contains an unrecognized format, the kernel leaves it unchanged and falls back to the RAM filesystem. This guard is not a substitute for backups.

## Known limitations

BreadOS does not yet provide user-mode processes, system calls, scheduling, privilege-isolated address spaces, a complete virtual-memory manager, a general-purpose on-disk filesystem, USB support, networking, Wi-Fi, audio, a graphical desktop, application compatibility, secure updates, or Android/ARM64 hardware support. The TSS and user-mode GDT entries are setup groundwork only; user-mode execution is not implemented.

The paging implementation maps only the first 16 MiB. The physical frame allocator therefore refuses to allocate frames outside that mapped region. The heap is a fixed, single-core arena and is not backed by the physical frame allocator.

## Project structure

- `boot.s`, `linker.ld`, `grub.cfg`: boot entry, memory layout, and GRUB configuration.
- `kernel.c`: kernel initialization and command shell.
- `memory.c`, `paging.c`, `heap.c`: physical-frame tracking, bootstrap paging, and heap.
- `gdt.c`, `gdt_load.s`, `interrupts.c`, `interrupts.s`: segmentation and interrupt setup.
- `kernel/keyboard.c`: PS/2 Set 1 keyboard translation and polling.
- `fs.c`, `pfs.c`, `ata.c`: RAM filesystem, BreadFS directory/path handling, and ATA PIO driver.
- `tests/filesystem-host.c`, `tests/keyboard-host.c`: host-side regression tests.
- `tests/boot-smoke.sh`: QEMU serial boot smoke test.
- `.github/workflows/build.yml`: CI build and smoke test.

## Contributing

Changes to kernel or storage behavior should be validated with a clean build and QEMU test. Treat disk images as disposable test fixtures and describe unimplemented functionality as planned work rather than an existing feature.
