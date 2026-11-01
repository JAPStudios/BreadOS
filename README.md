# BreadOS

BreadOS is a freestanding 32-bit x86 operating system with its own kernel and native system software. Omarchy is a feature reference; BreadOS does not use Linux, Arch, systemd, pacman, Wayland, Hyprland, Quickshell, D-Bus, or the Linux userspace ABI.

## Implemented

- Multiboot v1 boot through GRUB, VGA fallback, and validated 24/32-bit RGB framebuffer support.
- Native framebuffer drawing, glyph rendering, and graphical text output.
- GDT, TSS, IDT exceptions, remapped PIC, 100 Hz PIT, physical frame allocator, bootstrap paging, and fixed kernel heap.
- PS/2 keyboard IRQ1 input, PS/2 mouse IRQ12 packet decoding, bounded input queues, and serial diagnostics.
- RAM filesystem and ATA PIO-backed BreadFS v1. Storage is currently limited to 16 entries, 15-character paths, and 255-byte files.
- Round-robin kernel-thread scheduler and a preemptible ring-3 demonstration program using two user-mapped pages and an `int 0x80` write/exit syscall path.
- Native framebuffer desktop with draggable floating windows, four workspaces, dwindle/scroll/floating layouts, a dock, mouse interaction, and Shell, Files, Appearance, System, and Notifications windows.
- Twenty-three native color palettes ported from Omarchy theme data. Theme selection persists in BreadFS across reboots.
- Searchable command palette, 16-entry shell history, and a native notification service with a 16-entry history, stacked alerts, dismissal, snoozing, and Do Not Disturb.
- Host regression tests for filesystems, keyboard input, mouse packets, task switching, framebuffer rendering, themes, command search, and notifications, plus QEMU boot and persistence tests.

## Shell commands

`help`, `menu [SEARCH]`, `history`, `theme [NAME]`, `desktop`, `notify TEXT`, `notifications`, `snooze INDEX TICKS`, `dismiss INDEX`, `dnd [on|off]`, `about`, `video`, `userdemo`, `memory`, `storage`, `uptime`, `tasks`, `ls`, `mkdir PATH`, `rmdir PATH`, `cat PATH`, `write PATH TEXT`, `mv SOURCE DEST`, `rm PATH`, `clear`, `reboot`, and `halt`.

The `theme` command lists or applies the 23 palettes. `menu` searches the built-in command registry. `notify` creates a system notification; notification indexes are newest-first and snooze durations use PIT ticks (100 ticks = one second). `desktop` requires an RGB framebuffer of at least 320×240. Press `q` to return to the shell. Desktop shortcuts: `t` Appearance, `f` Files, `s` System, `e` Shell, `n` Notifications, `d` toggle Do Not Disturb, `l` cycle layouts, `,`/`.` scroll the scrolling layout, `1`–`4` or `[`/`]` switch workspaces.

## Build and test

On Debian/Ubuntu:

```sh
sudo apt-get update
sudo apt-get install build-essential binutils gcc-multilib grub-pc-bin grub-common xorriso mtools qemu-system-x86
make clean iso
make test
make run
```

## Remaining Omarchy feature work

The current desktop is kernel-rendered; its windows are not yet independent user-mode applications. The ring-3 path is a fixed smoke-test program, not a general process system. Independent address spaces, a general executable loader, IPC, process lifecycle management, a full VFS, per-monitor workspaces, multi-display management, advanced window effects, clipboard history, full settings panels, language/localization coverage, screen locking, idle management, screenshot/recording, audio, networking, Bluetooth, USB, broad hardware support, an installer, secure package management, transactional updates, snapshots, and the remaining Omarchy application/workflow features still require native implementation.

## Storage safety

`make run` uses the disposable `build/breados-disk.img` emulator image. The QEMU smoke test creates a zero-filled 16 MiB image. Do not point QEMU at a physical drive or an image containing data that must be preserved. BreadFS leaves unrecognized disk formats unchanged and falls back to the RAM filesystem.
