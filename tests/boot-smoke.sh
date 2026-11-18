#!/usr/bin/env bash
set -euo pipefail
mkdir -p build
# Create a fresh persistent volume containing the compiled native user application.
build/mk-breadfs-image build/breados-disk.img build/user/hello.elf
rm -f build/qemu-monitor.sock
timeout 12s qemu-system-i386 -display none \
  -monitor unix:build/qemu-monitor.sock,server,nowait -no-reboot -no-shutdown \
  -serial stdio -drive file=build/breados-disk.img,format=raw,if=ide,index=0 \
  -cdrom build/breados.iso > build/qemu-serial.log 2>&1 &
qemu_pid=$!
trap 'kill "$qemu_pid" 2>/dev/null || true' EXIT
python3 - <<'PY'
import os
import socket
import time

monitor_path="build/qemu-monitor.sock"
deadline=time.monotonic()+6.0
monitor=None
while time.monotonic()<deadline:
    try:
        candidate=socket.socket(socket.AF_UNIX,socket.SOCK_STREAM)
        candidate.connect(monitor_path)
        monitor=candidate
        break
    except OSError:
        try:
            candidate.close()
        except Exception:
            pass
        time.sleep(0.05)
if monitor is None:
    raise SystemExit("QEMU monitor did not become available")

# Wait until the shell prompt is visible before typing the application command.
deadline=time.monotonic()+6.0
while time.monotonic()<deadline:
    try:
        with open("build/qemu-serial.log",encoding="utf-8",errors="replace") as log:
            if "bread>" in log.read():
                break
    except OSError:
        pass
    time.sleep(0.05)

for key in ("e","x","e","c","spc","h","e","l","l","o","ret"):
    monitor.sendall(("sendkey "+key+"\n").encode("ascii"))
    time.sleep(0.15)
monitor.close()
PY
set +e
wait "$qemu_pid"
status=$?
set -e
trap - EXIT
if [ "$status" -ne 124 ]; then
  cat build/qemu-serial.log
  echo "QEMU exited unexpectedly with status $status" >&2
  exit 1
fi
grep -F "BreadOS kernel starting..." build/qemu-serial.log
grep -F "Boot OK. VGA console and PS/2 keyboard shell ready." build/qemu-serial.log
grep -F "Paging enabled: first 256 MiB identity-mapped." build/qemu-serial.log
grep -F "Persistent BreadFS mounted." build/qemu-serial.log
grep -F "bread>" build/qemu-serial.log
grep -F "BreadOS native user-mode application." build/qemu-serial.log
grep -F "Scheduler smoke test passed: round-robin task switching verified." build/qemu-serial.log
grep -F "URing-3 process and syscall smoke test passed." build/qemu-serial.log
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
