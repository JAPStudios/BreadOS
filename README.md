# BreadOS

BreadOS is an experimental freestanding 32-bit x86 kernel with a GRUB/Multiboot v1 boot path, VGA text output, serial diagnostics, and a polling PS/2 keyboard command shell.

## Build

On Debian or Ubuntu install build-essential, binutils, gcc-multilib, grub-pc-bin, grub-common, xorriso, and qemu-system-x86. Then run:

    make clean iso
    make run

The bootable image is build/breados.iso. The GitHub Actions workflow builds and uploads this ISO as an artifact.

## Shell commands

The initial shell supports help, about, clear, reboot, and halt. Keyboard support assumes a conventional US set-1 scancode layout and legacy PS/2 hardware.

## Limitations

This is a bootable kernel prototype, not a general-purpose or secure operating system. It does not yet implement paging/virtual memory, user mode, processes or scheduling, system calls, a persistent filesystem, USB input, networking, or a complete interrupt/exception subsystem. The keyboard loop polls the controller. A successful CI build verifies compilation and Multiboot structure; it does not by itself prove full runtime correctness on hardware.
