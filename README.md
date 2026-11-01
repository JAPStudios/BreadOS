# BreadOS

BreadOS is an experimental, freestanding 32-bit x86 operating-system kernel. It remains its own kernel and userspace; Omarchy is a feature reference, not a runtime base.

## Implemented kernel and system foundations

- Multiboot v1 boot through GRUB, retaining the VGA text console while accepting an RGB framebuffer when the bootloader supplies one.
- Validated Multiboot RGB framebuffer discovery and mapping into a dedicated virtual window, with clipped pixel and rectangle drawing primitives.
- GDT setup, TSS, IDT exception stubs, remapped PIC, and a 100 Hz PIT tick counter.
- Physical frame bitmap initialized from the Multiboot memory map; allocation remains restricted to the first 16 MiB identity-mapped region.
- Fixed 256 KiB kernel heap and bootstrap supervisor paging.
- Polling PS/2 keyboard shell and serial diagnostics.
- Small RAM filesystem and fixed-sector ATA PIO-backed BreadFS v1. Both remain limited to 16 total entries, 15-character paths, and 255 bytes per file.
- Round-robin kernel-thread scheduler groundwork.
- Host regression tests for filesystems, keyboard translation, task switching, and framebuffer drawing; QEMU boot/persistence smoke test.

The shell provides `help`, `about`, `video`, `memory`, `storage`, `uptime`, `tasks`, `ls`, `mkdir PATH`, `rmdir PATH`, `cat PATH`, `write PATH TEXT`, `mv SOURCE DEST`, `rm PATH`, `clear`, `reboot`, and `halt`.

## Build

On Debian/Ubuntu, install:

```sh
sudo apt-get update
sudo apt-get install build-essential binutils gcc-multilib grub-pc-bin grub-common xorriso mtools qemu-system-x86
```

Build and test:

```sh
make clean iso
make test
make run
```

## Omarchy feature port

The Omarchy project is being used to identify desktop, workflow, application, customization, installation, update, and quality-of-life features to reimplement natively. BreadOS must not acquire Linux, Arch, systemd, pacman, Wayland, Hyprland, Quickshell, D-Bus, or Linux userspace ABI dependencies.

The first native graphics foundation is now implemented in `video.c` and `paging.c`. This is not yet a graphical desktop, window manager, compositor, application framework, or full Omarchy port. Those require user-mode processes, syscalls, IPC, drivers, storage, and further graphics work.

## Known limitations

No user-mode execution, syscall ABI, process isolation, complete virtual-memory manager, general VFS, USB, networking, Wi-Fi, audio, GPU acceleration, window manager, compositor, application compatibility, secure package manager, transactional updates, installer, or Android/ARM64 support is implemented. The framebuffer renderer supports 24/32-bit RGB modes only and maps at most one 4 MiB physical window. The existing shell remains VGA text-based; the framebuffer currently provides graphics primitives and a minimal boot surface, not a complete desktop.

## Disk safety

`make run` uses `build/breados-disk.img`. The smoke test recreates this as a zero-filled 16 MiB image. Use disposable emulator images only; never point the emulator at a physical drive or an image containing data that must be preserved.

BreadFS initializes a disk only when its first sector is blank. If the first sector contains an unrecognized format, the kernel leaves it unchanged and falls back to the RAM filesystem.

## Third-party licensing

See [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md). Omarchy source code is MIT-licensed, but individual bundled fonts, wallpapers, icons, and other assets require separate license and provenance checks before reuse.
