#!/usr/bin/env bash
set -euo pipefail
mkdir -p build
# Create a fresh persistent volume containing the compiled native user application.
build/mk-breadfs-image build/breados-disk.img build/user/hello.elf build/user/guard.elf build/user/victim.elf build/user/attacker.elf build/user/wxguard.elf
rm -f build/qemu-monitor.sock
timeout 60s qemu-system-i386 -display none \
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

# Wait until the shell prompt is visible before typing application commands.
deadline=time.monotonic()+6.0
while time.monotonic()<deadline:
    try:
        with open("build/qemu-serial.log",encoding="utf-8",errors="replace") as log:
            if "bread>" in log.read():
                break
    except OSError:
        pass
    time.sleep(0.05)

def send_text(text):
    for character in text:
        key="spc" if character==" " else "ret" if character=="\n" else character
        monitor.sendall(("sendkey "+key+"\n").encode("ascii"))
        time.sleep(0.15)

def fail(message):
    try:
        with open("build/qemu-serial.log",encoding="utf-8",errors="replace") as log:
            print(log.read())
    except OSError:
        pass
    raise SystemExit(message)

def wait_for_text(needle,timeout=5.0):
    deadline=time.monotonic()+timeout
    while time.monotonic()<deadline:
        try:
            with open("build/qemu-serial.log",encoding="utf-8",errors="replace") as log:
                if needle in log.read():
                    return True
        except OSError:
            pass
        time.sleep(0.05)
    return False

send_text("exec hello\n")
if not wait_for_text("BreadOS native user-mode application."):
    raise SystemExit("Native hello application did not execute")

send_text("exec guard\n")
if not wait_for_text("BreadOS isolation test: attempting protected supervisor-memory write."):
    raise SystemExit("User-mode isolation test did not start")

# The protected write must fault and terminate only the user task. The shell
# must remain responsive, and the post-write failure marker must never appear.
send_text("help\n")
if not wait_for_text("Commands: help, menu"):
    raise SystemExit("Kernel did not recover to the shell after the user-mode protection fault")

# Two user processes map the same virtual data address to different frames.
send_text("exec victim\n")
time.sleep(0.5)
send_text("exec attacker\n")
if not wait_for_text("PASS: another user process could not modify victim memory."):
    fail("Separate user address spaces did not preserve victim memory")
send_text("help\n")
if not wait_for_text("Commands: help, menu"):
    fail("Kernel shell did not remain responsive after the process-isolation test")

# Writable data must not be executable even though this i386 target lacks NX.
send_text("exec wxguard\n")
if not wait_for_text("W^X test: attempting to execute writable data."):
    fail("W^X test application did not start")
send_text("help\n")
if not wait_for_text("Commands: help, menu"):
    fail("Kernel shell did not recover after the writable-execution fault")
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
grep -F "BreadOS isolation test: attempting protected supervisor-memory write." build/qemu-serial.log
grep -F "Commands: help, menu" build/qemu-serial.log
grep -F "PASS: another user process could not modify victim memory." build/qemu-serial.log
grep -F "W^X test: attempting to execute writable data." build/qemu-serial.log
grep -F "Commands: help, menu" build/qemu-serial.log
if grep -Fq "FAIL: another user process modified victim memory." build/qemu-serial.log || grep -Fq "FAIL: attacker could not send IPC to victim." build/qemu-serial.log || grep -Fq "FAIL: user CS limit includes writable data." build/qemu-serial.log || grep -Fq "FAIL: user CS descriptor could not be inspected." build/qemu-serial.log || grep -Fq "FAIL: writable data executed as code." build/qemu-serial.log; then
  grep -E "FAIL: (another user process|attacker could not|user CS|writable data)" build/qemu-serial.log >&2 || true
  echo "User processes were not isolated correctly." >&2
  exit 1
fi
if grep -Fq "FAIL: user mode modified supervisor memory." build/qemu-serial.log; then
  echo "User-mode process wrote to supervisor memory." >&2
  exit 1
fi
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
