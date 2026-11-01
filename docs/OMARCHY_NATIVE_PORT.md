# Omarchy feature extraction and native BreadOS port

Status: architecture and feature inventory. This document is not a claim that the listed desktop features are already implemented.

## Non-negotiable architecture

BreadOS remains its own freestanding kernel. The Omarchy repository is a reference for user experience, desktop features, workflows, configuration, and operational quality. It is not a kernel, bootloader, package base, or userspace to install into BreadOS.

Do not introduce Linux, an Arch package root, the Linux system-call ABI, systemd, pacman, Wayland, Hyprland, Quickshell, D-Bus, or shell-script assumptions as BreadOS runtime dependencies. A host may use ordinary development tools to build BreadOS, but the booted operating system must run on BreadOS kernel services.

The current BreadOS implementation is a 32-bit x86 Multiboot v1 kernel. It has a VGA text console, polling PS/2 keyboard input, GDT/IDT/PIC/PIT setup, a bootstrap identity map limited to 16 MiB, a small fixed heap, a primitive kernel-thread scheduler, a RAM filesystem, and a small ATA PIO-backed BreadFS. It does not yet have a framebuffer graphics stack, user-mode execution, syscalls, process isolation, a general VFS, USB, networking, audio, or a graphical desktop. These missing foundations are prerequisites, not details that can be solved by copying Omarchy scripts.

## Upstream extraction snapshot

Source: https://github.com/omacom/omarchy, default branch `quattro`, inspected at commit `d367a568d97ff4579f85d7e33425c97cfae717b4`.

The recursive upstream Git tree contained 5,705 paths at that revision. Its major source-tree counts were:

| Upstream area | Paths | What it represents | BreadOS treatment |
| --- | ---: | --- | --- |
| `bin/` | 567 | User-facing commands and system helpers | Recreate command behavior as native kernel/user services and built-in utilities; do not port Bash scripts as executable dependencies |
| `default/` | 604 | Default configuration, fonts, shell and system defaults | Define BreadOS configuration schemas and native defaults; audit each asset's separate license |
| `manual/` | 2,644 | 51 canonical manual chapters plus localized copies | Extract feature requirements and write original BreadOS documentation; do not blindly import translations or upstream branding |
| `test/` | 569 | Unit, integration, shell and graphical acceptance tests | Translate behavior into host tests, kernel tests, emulator tests and hardware acceptance tests |
| `themes/` | 336 | Theme definitions, previews, backgrounds and related assets | Build a native theme format and renderer; review every image/font/icon license before copying |
| `shell/` | 321 | Quickshell desktop implementation and plugins | Reimplement the shell as a native BreadOS compositor/window manager and desktop services |
| `migrations/` | 220 | Versioned configuration and system migrations | Create transactional BreadOS settings and filesystem migrations |
| `install/` | 142 | Installation, provisioning, hardware and package setup | Rebuild as a BreadOS installer, device discovery and native provisioning system |
| `config/` | 81 | Application and desktop configuration | Convert relevant settings into typed BreadOS configuration |
| `applications/` | 34 | Desktop-entry definitions and icons | Define a BreadOS application manifest and launcher registry |
| `docs/` | 16 | Engineering documentation for Omarchy internals | Use as design input; keep BreadOS implementation documentation independent |

The upstream tree also contains `etc/`, `agents/`, `scripts/`, `plans/`, `oligarchy-tests/`, and other project metadata. Counts describe paths in the inspected source tree, not independent features or ready-to-port programs.

The upstream manual's 51 canonical topics cover welcome/setup, navigation, top bar, themes, hotkeys, clipboard history, reminders, notifications, text extraction/dictation, screenshots/recording, toggles/idle/screensaver, command-line tools, terminal/editor/development workflows, TUIs/GUIs, browsers, web apps, gaming, PDF workflows, updates, dotfiles, shell plugins, displays, keyboard/mouse/trackpad, networking, sleep, hardware authentication, fonts, backgrounds, branding, customization, troubleshooting, snapshots, security, dual boot, and unattended installation. The application chapters are examples of an ecosystem, not a requirement to bundle every proprietary service or application.

## Native BreadOS feature map

### 1. Desktop and interaction

| Omarchy capability | Native BreadOS component | Required foundation |
| --- | --- | --- |
| Tiling/floating windows, focus, workspaces, layouts | BreadWM | Framebuffer, input events, user processes, IPC, compositor |
| Top bar, status modules, app launcher, system menu | BreadPanel and BreadMenu | Window surfaces, fonts, icons, application registry |
| Notifications and Do Not Disturb | BreadNotify | IPC, timers, persistent user state, window surfaces |
| Clipboard history | BreadClipboard | Keyboard/input events, process IPC, bounded persistent store |
| Hotkeys and keybinding layers | BreadInput | Event-driven keyboard and pointer drivers |
| Screenshots and screen recording | BreadCapture | Framebuffer readback; video encoder and storage later |
| Idle, lock screen, screensaver | BreadSession | Timers, process/session model, secure input focus |
| Multiple displays, rotation, scaling | BreadDisplay | PCI/device discovery, GPU/display drivers, modesetting |

### 2. Appearance and customization

| Omarchy capability | Native BreadOS component | Required foundation |
| --- | --- | --- |
| Color themes and coordinated application colors | BreadTheme | Typed theme schema and renderer palette |
| Wallpapers and background rotation | BreadWallpaper | Image decoder, framebuffer compositor, persistent settings |
| Fonts and icon sets | BreadFont and BreadIcon | Font rasterizer, glyph atlas, image decoder |
| Prompt, terminal and editor appearance | Native app theme adapters | BreadOS terminal and app configuration |
| User overrides, plugins and hooks | BreadConfig and BreadExtensions | Permission model, IPC, extension ABI, safe loader |

Themes must be data, not executable scripts. A theme cannot run privileged code or change arbitrary kernel state.

### 3. Applications and development environment

| Omarchy capability | Native BreadOS component | Required foundation |
| --- | --- | --- |
| Terminal and shell tools | BreadTerminal and BreadShell | User mode, syscalls, process lifecycle, PTY/console API |
| Text editor and developer tools | Native app ecosystem | Executable format, filesystem, memory protection, compiler/runtime ports |
| Application launcher and desktop entries | BreadApps | Application manifest, process creation, icons and windowing |
| Browser and web applications | Browser port or native browser project | Networking, TLS, DNS, TCP/IP, graphics, font and media stacks |
| Gaming and multimedia | Native runtime and device APIs | GPU, audio, input, storage, scheduler, graphics APIs |
| PDF, image, video and capture workflows | Native applications/services | File chooser, codecs, image and font libraries, sandboxing |

Applications are not “ported” merely by copying their desktop-entry files. Each application needs a compatible executable and the system APIs it expects. Proprietary services remain optional integrations and cannot be represented as included applications without their binaries and licenses.

### 4. Installation, updates and system management

| Omarchy capability | Native BreadOS component | Required foundation |
| --- | --- | --- |
| Guided and unattended installation | BreadSetup | Disk enumeration, partitioning, filesystem creation, boot installation, recovery |
| Hardware detection and provisioning | BreadHardware | PCI enumeration, ACPI, device drivers, hardware identity |
| Package installation and removal | BreadPkg | Signed package format, dependency solver, file ownership, rollback |
| System and application updates | BreadUpdate | Signed manifests, atomic install/rollback, versioned migrations |
| Configuration refresh and migrations | BreadMigrate | Typed config store, schema versions, atomic filesystem operations |
| Snapshots and recovery | BreadSnapshot | Copy-on-write or journaled filesystem, bootable recovery |
| Security settings and authentication | BreadSecurity | User accounts, credentials, privilege separation, secure boot/update chain |
| Power, sleep and battery status | BreadPower | ACPI/power-management drivers and device events |
| Networking, Bluetooth and audio | BreadNetwork, BreadBluetooth, BreadAudio | Native protocol stacks and hardware drivers |

No package manager or updater should execute untrusted package scripts as root. Packages must be signed, integrity checked, staged, and recoverable after power loss. The updater must reject downgrade and rollback attacks.

### 5. Documentation and operational quality

The reference project has detailed end-user and developer documentation, command metadata, install guidance, migrations, tests, and visual verification procedures. BreadOS should reproduce those *quality practices* with:
- a user manual organized by actual BreadOS capabilities;
- API and driver documentation;
- deterministic host-side unit tests;
- QEMU boot, input, graphics, filesystem and recovery tests;
- hardware compatibility records;
- UI screenshots and interaction acceptance tests;
- explicit release notes and migration history.

## Implementation order and exit criteria

This is a dependency order. A later layer must not be declared complete merely because its files exist.

1. **Kernel safety and memory:** complete physical-memory discovery and mapping, page permissions, allocator-backed heap, interrupt-safe synchronization, robust scheduler and crash diagnostics. Exit: allocator and scheduler stress tests, guard-page tests, no unexplained faults.
2. **Processes and system calls:** ring-3 transition, per-process address spaces, syscall ABI, executable loader, process lifecycle, IPC, permissions and user/kernel copy validation. Exit: isolated user programs cannot read/write kernel memory or another process.
3. **Storage and device model:** general VFS, mount points, block-device layer, PCI enumeration, ACPI discovery, robust disk drivers and input event queues. Exit: filesystem corruption tests, multi-device enumeration, safe error handling.
4. **Graphics and input:** boot framebuffer discovery, pixel formats, drawing primitives, font rendering, image decoding, pointer driver, keyboard event model and compositor. Exit: deterministic framebuffer tests and QEMU interaction tests.
5. **Desktop shell:** window manager, focus model, workspaces, panel, launcher, menus, notifications, settings, clipboard and hotkeys. Exit: multiple isolated user processes can open, move, resize, focus and close windows without corrupting the display or kernel.
6. **Themes and user customization:** validated theme schema, wallpapers, fonts, icon resources, per-user settings, extension permissions. Exit: switching themes changes all supported native surfaces and malformed themes cannot crash the shell.
7. **Installer and lifecycle:** partitioning, formatting, account setup, boot setup, signed packages, transactional updates, migrations, snapshots and recovery. Exit: install/update interruption tests and successful recovery from an intentionally interrupted transaction.
8. **Applications and connectivity:** terminal, editor, file manager, browser/runtime, networking, audio, USB, printing, media, development tools and games. Each application is accepted only when its dependencies and compatibility tests are present.
9. **Release engineering:** reproducible builds, signed releases, hardware matrix, security review, accessibility and localization.

## Porting rules

- Preserve the behavior and user value of a feature, not its Linux implementation details.
- Keep the BreadOS kernel independent. Never add a Linux syscall shim or bundle a Linux kernel to make an upstream component appear to work.
- Do not copy shell scripts, QML, binaries, fonts, icons, wallpapers or other assets wholesale. Port source only where it is technically compatible and retain required notices; check asset-specific licenses separately.
- Keep privileged operations behind narrow, auditable kernel or system-service interfaces.
- Use bounded inputs, checked arithmetic, explicit ownership and failure paths in kernel-facing code.
- Mark every feature as planned, in progress, implemented, tested, or hardware-verified. Do not call a feature complete because a stub or design document exists.

## Upstream references

- Omarchy source: https://github.com/omacom/omarchy
- Omarchy manual: https://github.com/omacom/omarchy/tree/quattro/manual
- Omarchy engineering docs: https://github.com/omacom/omarchy/tree/quattro/docs
- Omarchy license: https://github.com/omacom/omarchy/blob/quattro/LICENSE
