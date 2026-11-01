# BreadOS

BreadOS is an experimental, freestanding 32-bit x86 operating-system kernel. It remains its own kernel and userspace; Omarchy is a feature reference, not a runtime base.

## Implemented kernel and system foundations

- Multiboot v1 boot through GRUB, retaining the VGA text console while accepting an RGB framebuffer when the bootloader supplies one.
- Validated Multiboot RGB framebuffer discovery and mapping into a dedicated virtual window, with a native 5×7 glyph renderer, text console, clipped pixel and rectangle drawing primitives.
- GDT setup, TSS, IDT exception stubs, remapped PIC, and a 100 Hz PIT tick counter.
- Physical frame bitmap initialized from the Multiboot memory map; allocation remains restricted to the first 16 MiB identity-mapped region.
- Fixed 256 KiB kernel heap and bootstrap supervisor paging.
- PS/2 keyboard IRQ1 character queue with polling fallback, plus PS/2 mouse IRQ12 packet decoding and bounded pointer-event queue; serial diagnostics.
- Small RAM filesystem and fixed-sector ATA PIO-backed BreadFS v1. Both remain limited to 16 total entries, 15-character paths, and 255 bytes per file.
- Round-robin kernel-thread scheduler with a preemptible ring-3 demonstration process, two user-mapped pages, and an `int 0x80` character-write/exit syscall path.
- Native framebuffer desktop with draggable floating windows, four workspaces, a dock, mouse interaction, and built-in Shell, Files, Appearance, and System windows. The Appearance window selects themes; Files lists BreadFS entries; System displays live kernel counters.\n- Native command palette with category and text search, 16-entry shell history, and 23 selectable Omarchy-derived color themes. Theme selection persists across boots on BreadFS.
- Host regression tests for filesystems, keyboard translation and input-queue behavior, mouse packet decoding, task switching, framebuffer drawing/text, theme selection, and command-palette search; QEMU boot/persistence smoke test.

The shell provides `help`, `menu [SEARCH]`, `history`, `theme [NAME]`, `about`, `video`, `userdemo`, `memory`, `storage`, `uptime`, `tasks`, `ls`, `mkdir PATH`, `rmdir PATH`, `cat PATH`, `write PATH TEXT`, `mv SOURCE DEST`, `rm PATH`, `clear`, `reboot`, and `halt`. `menu` searches a native command registry; `theme` lists or applies the 23 palettes and saves the selected theme in BreadFS; `desktop` opens the framebuffer desktop (press `q` to return, use `t`, `f`, `s`, `e`, `1`–`4`, and `[`/`]` for navigation).

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

## Native desktop feature port

BreadOS reimplements Omarchy's user-facing features against its own kernel and userspace. The first native ports are the searchable command palette, shell history, and persistent theme selection using all 23 upstream theme color palettes. The framebuffer console, keyboard, mouse, and ring-3 syscall foundation are also native BreadOS code. Linux, Arch, systemd, pacman, Wayland, Hyprland, Quickshell, D-Bus, and Linux userspace ABI components are not runtime dependencies. This remains an incremental implementation: the full compositor and process-backed application framework, per-monitor workspaces, tiling and scrolling layouts, notifications, clipboard manager, full settings panels, installer, package/update system, networking, audio, and broad hardware support are still outstanding.

## Known limitations

The only ring-3 execution path is a fixed smoke-test program with two user pages and two system calls. The QEMU boot test checks that it survives timer preemption, writes through `int 0x80`, and exits back to the kernel shell. BreadOS does not yet have independent per-process page directories, a general executable loader, process lifecycle management, IPC, a general syscall ABI, or user account/permission services. A complete virtual-memory manager, general VFS, USB, networking, Wi-Fi, audio, GPU acceleration, window manager, compositor, application compatibility, secure package manager, transactional updates, installer, and Android/ARM64 support are also not implemented. The framebuffer renderer supports 24/32-bit RGB modes only and maps at most one 4 MiB physical window. The shell can render through the framebuffer text console when GRUB supplies a supported RGB mode, and falls back to VGA text otherwise. No windowed desktop exists yet.

## Disk safety

`make run` uses `build/breados-disk.img`. The smoke test recreates this as a zero-filled 16 MiB image. Use disposable emulator images only; never point the emulator at a physical drive or an image containing data that must be preserved.

BreadFS initializes a disk only when its first sector is blank. If the first sector contains an unrecognized format, the kernel leaves it unchanged and falls back to the RAM filesystem.

