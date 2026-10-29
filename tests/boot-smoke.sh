#!/usr/bin/env bash
set -euo pipefail
mkdir -p build
# A new zero-filled disk lets the guest initialize BreadFS without touching host data.
truncate -s 16M build/breados-disk.img
set +e
timeout 8s qemu-system-i386 -display none -monitor none -no-reboot -no-shutdown -serial stdio \
  -drive file=build/breados-disk.img,format=raw,if=ide,index=0 -cdrom build/breados.iso > build/qemu-serial.log 2>&1
status=$?
set -e
if [ "$status" -ne 124 ]; then
  cat build/qemu-serial.log
  echo "QEMU exited unexpectedly with status $status" >&2
  exit 1
fi
grep -F "BreadOS kernel starting..." build/qemu-serial.log
grep -F "Boot OK. VGA console and PS/2 keyboard shell ready." build/qemu-serial.log
grep -F "Paging enabled: first 16 MiB identity-mapped." build/qemu-serial.log
grep -F "Persistent BreadFS formatted and mounted." build/qemu-serial.log
grep -F "bread>" build/qemu-serial.log
grep -F "Scheduler smoke test passed: round-robin task switching verified." build/qemu-serial.log
# Boot a second time without replacing the image; the volume must mount, not format.
set +e
timeout 8s qemu-system-i386 -display none -monitor none -no-reboot -no-shutdown -serial stdio \
  -drive file=build/breados-disk.img,format=raw,if=ide,index=0 -cdrom build/breados.iso > build/qemu-serial-second.log 2>&1
status=$?
set -e
if [ "$status" -ne 124 ]; then
  cat build/qemu-serial-second.log
  echo "Second QEMU boot exited unexpectedly with status $status" >&2
  exit 1
fi
grep -F "Persistent BreadFS mounted." build/qemu-serial-second.log
