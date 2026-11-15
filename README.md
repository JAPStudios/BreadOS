# BreadOS

BreadOS is a freestanding 32-bit x86 operating system with its own kernel and native system software. Omarchy is a feature reference; BreadOS does not use Linux, Arch, systemd, pacman, Wayland, Hyprland, Quickshell, D-Bus, or the Linux userspace ABI.

## Implemented

- Multiboot v1 boot through GRUB, VGA fallback, and validated 24/32-bit RGB framebuffer support.
- Native framebuffer drawing, glyph rendering, and graphical text output.
- GDT, TSS, IDT exceptions, remapped PIC, 100 Hz PIT, physical frame allocator, bootstrap paging, and a hardened fixed kernel heap with allocation canaries, guarded block metadata, allocation/free poisoning, and an integrity checker.
- PS/2 keyboard IRQ1 input, PS/2 mouse IRQ12 packet decoding, bounded input queues, and serial diagnostics.
- RAM filesystem and ATA PIO-backed BreadFS v1, exposed through a mounted native VFS. The VFS mounts the active root backend at `/`, device nodes at `/dev`, and live system information at `/proc`; it provides file handles, sequential reads/writes, seeking, metadata queries, directory enumeration, file/directory operations, plus console, serial, null, and zero devices. Shell, desktop file browsing, and executable loading use the VFS. The persistent volume has 16 metadata entries and 15-character paths. Text files hold up to 255 bytes; binary files have separate 65,535-byte extents and require a disk of at least 4,096 sectors (2 MiB). Binary writes are not yet transactional.
- Round-robin scheduler for three kernel tasks plus up to five independently scheduled ring-3 processes. Each user process has a private page directory, mapped ELF load segments, a user stack, a dedicated kernel interrupt stack, and an `int 0x80` write/exit/IPC syscall path. User tasks exchange bounded 64-byte messages through eight-slot nonblocking mailboxes; user pointers are validated against mapped user pages before copying. The scheduler preserves each process interrupt frame and address space; exit and recoverable user exceptions move the process to a zombie state, then reclaim its page frames and stacks after switching away from its address space.
- Native framebuffer desktop with draggable floating windows, four workspaces, dwindle/scroll/floating layouts, a dock, mouse interaction, and Shell, Files, Appearance, System, Notifications, and Clipboard windows. The Files window previews selected BreadFS text files.
- Twenty-three native color palettes ported from Omarchy theme data. Theme selection persists in BreadFS across reboots.
- Searchable command palette, 16-entry shell history, and a native notification service with a 16-entry history, stacked alerts, dismissal, snoozing, and Do Not Disturb.
- Native clipboard history with sensitive-content exclusion and compiler-resistant memory wiping when clipboard contents are cleared, plus timer-based reminders delivered through the notification service.
- Configurable desktop idle screensaver with a persistent timeout; keyboard or mouse activity wakes it. This is not an authenticated screen lock.
- Native privacy permission policy for camera, microphone, location, network, Bluetooth, USB, and sensors; permissions default to denied and persist in BreadFS. This is a policy registry, not hardware isolation: device drivers and general user processes are not yet wired to enforce these permissions.
- Host regression tests for filesystem metadata, binary storage, VFS mounts and file operations, ELF validation, PCI and audio helpers, keyboard input, mouse packets, task switching, user-task lifecycle, IPC queues, framebuffer rendering, themes, command search, notifications, clipboard history, reminders, privacy settings, secure memory, and idle timeout behavior, plus a QEMU boot/persistence smoke test.

## Shell commands

`help`, `menu [SEARCH]`, `history`, `heapcheck`, `theme [NAME]`, `desktop`, `notify TEXT`, `notifications`, `snooze INDEX TICKS`, `dismiss INDEX`, `dnd [on|off]`, `clipboard copy|sensitive|paste|history|clear`, `remind TICKS TEXT`, `reminders`, `remind clear`, `idle [TICKS|off|status]`, `privacy [list|CAPABILITY on|off]`, `beep FREQUENCY [DURATION_TICKS]`, `about`, `video`, `userdemo`, `exec PATH`, `memory`, `storage`, `uptime`, `tasks`, `ps`, `ls [PATH]`, `mkdir PATH`, `rmdir PATH`, `cat PATH`, `write PATH TEXT`, `mv SOURCE DEST`, `rm PATH`, `clear`, `reboot`, and `halt`. The `exec` command loads an ELF32/i386 binary through the VFS binary-file API. `ps` lists live user task IDs. User-mode syscall registers are: `eax=1` writes the low byte of `ebx`; `eax=2` exits with `ebx` as the status; `eax=3` sends a message with `ebx` target, `ecx` user buffer, and `edx` length; `eax=4` receives into the `ebx` user buffer with `ecx` capacity and returns the sender in `edx`; `eax=5` returns the current task ID. IPC send and receive are nonblocking.

The `beep` command plays a tone through the PC speaker using PIT channel 2 (20–20,000 Hz; duration in 100 Hz timer ticks); this is basic mono tone output, not a sound-card, microphone, mixer, or audio-visualization stack. The `heapcheck` command validates the kernel heap block chain, metadata guards, and live allocation canaries. The allocator refuses to free allocations whose canaries fail validation. The `idle` command configures the desktop screensaver timeout in PIT ticks (100 ticks = one second, default five minutes; `off` disables it). The screensaver wakes on keyboard or mouse activity and is not a security lock screen. The `theme` command lists or applies the 23 palettes. `menu` searches the built-in command registry. `notify` creates a system notification; notification indexes are newest-first and snooze durations use PIT ticks (100 ticks = one second). `desktop` requires an RGB framebuffer of at least 320×240. Press `q` to return to the shell. Desktop shortcuts: `t` Appearance, `f` Files, `s` System, `e` Shell, `n` Notifications, `c` Clipboard, `d` toggle Do Not Disturb, `l` cycle layouts, `,`/`.` scroll the scrolling layout, `1`–`4` or `[`/`]` switch workspaces.

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

The current desktop is kernel-rendered; its windows are not yet independent user-mode applications. The kernel now supports independently scheduled ELF32/i386 user processes with per-process kernel stacks, exit handling, deferred resource reclamation, and bounded nonblocking IPC. The VFS still needs per-process file descriptor tables, access-control enforcement, symlinks, mount lifecycle management, and broader storage-driver support. Per-monitor workspaces, multi-display management, advanced window effects, cross-application clipboard integration, full settings panels, language/localization coverage, authenticated screen locking, screenshot/recording, full sound-card audio, networking, Bluetooth, USB, broad hardware support, an installer, secure package management, transactional updates, snapshots, and the remaining Omarchy application/workflow features still require native implementation. The privacy permission registry is not yet enforced by device drivers or a general user-process security boundary.

## Storage safety

`make run` uses the disposable `build/breados-disk.img` emulator image. The QEMU smoke test creates a zero-filled 16 MiB image. Do not point QEMU at a physical drive or an image containing data that must be preserved. BreadFS leaves unrecognized disk formats unchanged and falls back to the RAM filesystem.
