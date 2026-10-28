#!/usr/bin/env bash
set -euo pipefail
mkdir -p build
set +e
timeout 8s qemu-system-i386 -display none -monitor none -no-reboot -no-shutdown -serial stdio -cdrom build/breados.iso > build/qemu-serial.log 2>&1
status=$?
set -e
# timeout(1) uses 124 when QEMU remains running; with -no-shutdown QEMU
# should remain alive even if the guest requests shutdown.
if [ "$status" -ne 124 ]; then
  cat build/qemu-serial.log
  echo "QEMU exited unexpectedly with status $status" >&2
  exit 1
fi
grep -F "BreadOS kernel starting..." build/qemu-serial.log
grep -F "Boot OK. VGA console and PS/2 keyboard shell ready." build/qemu-serial.log
grep -F "bread>" build/qemu-serial.log
