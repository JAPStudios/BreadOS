from pathlib import Path
p=Path(__file__).resolve().parent/'disk.img'
try:
    with p.open('xb') as f:
        f.truncate(1024*1024)
except FileExistsError:
    if p.stat().st_size < 1024*1024:
        raise SystemExit('Existing disk.img is too small; not modified.')
    print('Existing disk.img preserved.')
else:
    print('Created dedicated 1 MiB disk.img for emulator testing.')
