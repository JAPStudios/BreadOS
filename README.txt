INTEGRATED OPERATING-SYSTEM STARTER SOURCE

Extract into a NEW empty directory. This is a standalone consolidated
prototype, not a patch to any existing project or repository.
No dependency on previous ZIPs. All runtime C modules are in kernel.c.

Target: 32-bit x86; legacy BIOS; GRUB Multiboot; QEMU i386; VGA text;
BIOS-initialized emulated PS/2 keyboard; primary-master IDE ATA PIO disk.
No native UEFI, ARM, AHCI, NVMe, USB, GUI, networking, separate programs,
process isolation, malware detection, or universal application compatibility.
It is not the complete operating system described in the funding vision.

BUILD PREREQUISITES
- i686-elf GCC/binutils cross-toolchain
- GNU Make
- grub-file, grub-mkrescue and GRUB BIOS modules
- xorriso
- qemu-system-i386
- Python 3

BUILD AND BOOT
  make run

FILE-MOVEMENT DEMONSTRATION (inside the kernel shell)
  mkdir "/My Documents"
  touch "/my notes.txt"
  write "/my notes.txt" "Hello"
  mv "/my notes.txt" "/My Documents/"
  ls "/My Documents"
  cat "/My Documents/my notes.txt"
  mv "/My Documents/my notes.txt" /
  cp "/my notes.txt" "/My Documents/copy.txt"
  save
  halt

Close QEMU. Run make run again, then enter:
  load
  cat "/my notes.txt"

save writes the whole RAM store to disk.img. load replaces live RAM files
with that snapshot and resets the current directory to /. Save and load
are explicit; no automatic disk write or startup restore occurs.

This build uses a NEW snapshot magic value. Do not reuse disk.img from
earlier prototypes. Preserve old images separately. It cannot import old
snapshots or files directly from the host computer or GitHub.

LIMITS AND SAFETY
63 user entries; 31-byte names; 1023-byte text capacity per file; 255-character
shell input; 16 arguments. cp copies files only. rm refuses nonempty folders
and current-directory ancestors. Existing destinations are not overwritten.
Absolute/relative paths, . and .., single/double quotes, backslash escaping
are implemented. No Bash expansion, pipes, redirection, permissions or ACLs.

Never use a physical disk: code overwrites sector zero and following sectors
on its selected ATA disk. Only use the dedicated QEMU disk.img.
Save is NOT crash-atomic; interrupted saving can destroy the old snapshot.
Keep backups while QEMU is closed. Checksum is not cryptographic protection.
make clean preserves disk.img. Removing it removes its saved contents.
Generated source is a development prototype; see VERIFICATION.txt.
