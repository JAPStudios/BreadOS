# BreadOS

BreadOS is an experimental freestanding 32-bit x86 kernel. It boots through GRUB/Multiboot v1 and currently provides VGA text output, serial diagnostics, a polling legacy PS/2 keyboard shell, CPU exception gates, and a PIT timer interrupt at approximately 100 Hz.

## Build and run

On Debian or Ubuntu:

    sudo apt-get update
    sudo apt-get install -y build-essential binutils gcc-multilib grub-pc-bin grub-common xorriso mtools qemu-system-x86
    make clean iso
    make run

The bootable image is build/breados.iso. In QEMU, the shell currently accepts help, about, clear, reboot, and halt.

## Automated checks

GitHub Actions compiles and links the kernel, validates its Multiboot header, creates the ISO, and boots it briefly in QEMU to verify the serial-console startup message. This is a boot smoke test, not a full hardware or reliability test.

## Scope and limitations

This is not yet a general-purpose operating system and does not replace Android, Windows, macOS, Linux, or other OSes. It currently targets 32-bit x86 PCs and emulators. It does not yet implement paging/virtual memory, user mode, process scheduling, system calls, a persistent filesystem, USB, networking, a complete hardware driver model, a graphics desktop, or application compatibility. The PS/2 keyboard is polled, and only the PIT IRQ is enabled. No physical-hardware compatibility is implied by the CI result.
