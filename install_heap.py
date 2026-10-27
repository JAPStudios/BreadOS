from pathlib import Path
import shutil
p=Path(__file__).resolve().parent
for name in ['kernel.c','Makefile','heap.c','heap.h']:
    if not (p/name).is_file():raise SystemExit('Missing '+name)
k=(p/'kernel.c').read_text();m=(p/'Makefile').read_text()
if '#include "heap.h"' in k or 'heap.o' in m:raise SystemExit('Already present; no changes made.')
anchor='void kernel_main(void){'
if anchor not in k or 'boot.o kernel.o' not in m:raise SystemExit('Unexpected source layout; no changes made.')
for name in ['kernel.c','Makefile']:
    if (p/(name+'.before-heap')).exists():raise SystemExit('Backup already exists; no changes made.')
k='#include "heap.h"\n'+k.replace(anchor,anchor+'\n heap_init();',1)
m=m.replace('boot.o kernel.o','boot.o kernel.o heap.o')
m+='\nheap.o: heap.c heap.h\n\t$(CC) $(CFLAGS) -c heap.c -o heap.o\n'
for name in ['kernel.c','Makefile']:shutil.copy2(p/name,p/(name+'.before-heap'))
(p/'kernel.c').write_text(k);(p/'Makefile').write_text(m)
print('Heap integrated and initialized. No new shell command was added.')
