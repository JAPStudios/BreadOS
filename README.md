# BreadOS

BreadOS is an operating-system project. The current code is an early **32-bit x86 BIOS/GRUB prototype**, not yet an Android replacement and not yet a mobile operating system.

## One source of truth

The modular kernel in [`kermal/`](kermal/) is the active starter:
- `kernel.c`: kernel entry and startup
- `terminal.c/.h`: VGA text terminal
- `keyboard.c/.h`: polling PS/2 keyboard input
- `shell.c/.h`: interactive command shell
- `boot.s`, `linker.ld`, `grub.cfg`: boot and link configuration
- `Makefile`: build and QEMU run targets

The older standalone prototype and its experimental scripts remain at the repository root for reference. They are **not** the build path for the modular kernel. Do not mix root-level prototype files into the `kermal/` build.

## Build and run

On a Linux machine with GCC supporting 32-bit freestanding compilation, GNU Make, GRUB BIOS tools, xorriso, and QEMU installed:

```sh
cd kermal
make
make run
```

To remove generated build outputs:

```sh
make clean
```

The expected ISO is `kermal/build/breados.iso`. Build success is not proof of a successful boot; boot behavior must be tested in QEMU.

## Current scope

The modular kernel currently targets legacy BIOS/GRUB Multiboot v1, 32-bit x86, VGA text output, and polling PS/2 keyboard input. It has a small interactive shell with `help`, `about`, `clear`, and `halt`.

It does **not** currently provide a graphical interface, touchscreen support, ARM/ARM64 support, Android application compatibility, phone hardware drivers, Wi-Fi/mobile networking, audio, power management, secure app isolation, or a mobile installer. These are future engineering tasks, not implemented features.

## Android-replacement direction

A phone-capable system requires a target device and its boot chain, a supported CPU architecture, display/touch/input drivers, storage, networking, audio, power management, a graphical system, application APIs, security boundaries, and an install/recovery/update path. The current x86 prototype is groundwork for OS development, not a substitute for those components.
