from pathlib import Path
import shutil
root = Path(__file__).resolve().parent
required = ['fs.c','fs.h','shell.c','Makefile','ata.c','ata.h','fs_persistence.inc']
for name in required:
    if not (root/name).is_file():
        raise SystemExit('Missing '+name+'. Extract this update into the previous filesystem project directory.')
original = {name:(root/name).read_text() for name in ['fs.c','fs.h','shell.c','Makefile']}
if 'fs_persistence.inc' in original['fs.c']:
    raise SystemExit('Update already applied; no changes made.')
if 'typedef struct { int used, directory, parent; char name[NAME], data[DATA]; } Node;' not in original['fs.c']:
    raise SystemExit('Unexpected fs.c layout; no changes made.')
if 'static Node nodes[COUNT];' not in original['fs.c']:
    raise SystemExit('Expected filesystem storage missing; no changes made.')
anchor='  if(eq(cmd,"help"))'
if anchor not in original['shell.c']:
    raise SystemExit('Unexpected shell.c; no changes made.')
if 'fs.o' not in original['Makefile'] or 'qemu-system-i386 -cdrom starter.iso' not in original['Makefile']:
    raise SystemExit('Unexpected Makefile; no changes made.')
for name in original:
    if (root/(name+'.before-persistence')).exists():
        raise SystemExit('Backup already exists for '+name+'; no changes made.')
updated = dict(original)
updated['fs.c'] += '\n#include "fs_persistence.inc"\n'
pos=updated['fs.h'].rfind('#endif')
if pos < 0:
    raise SystemExit('Unexpected fs.h; no changes made.')
updated['fs.h']=updated['fs.h'][:pos]+'int fs_save(void);\nint fs_load(void);\n'+updated['fs.h'][pos:]
commands="""  if(eq(cmd,"save")){terminal_write(fs_save()==0?"Saved to disk image.\\n":"Save failed.\\n");continue;}
  if(eq(cmd,"load")){terminal_write(fs_load()==0?"Loaded snapshot; current directory is /.\\n":"Load failed; RAM files unchanged.\\n");continue;}
"""
updated['shell.c']=updated['shell.c'].replace(anchor,commands+anchor,1)
updated['shell.c']=updated['shell.c'].replace('help clear about echo halt','help clear about echo halt save load')
updated['Makefile']=updated['Makefile'].replace('shell.o fs.o','shell.o fs.o ata.o')
updated['Makefile']=updated['Makefile'].replace('run: iso','run: iso disk.img')
updated['Makefile']=updated['Makefile'].replace('qemu-system-i386 -cdrom starter.iso','qemu-system-i386 -boot d -cdrom starter.iso -drive file=disk.img,format=raw,if=ide,index=0,media=disk')
updated['Makefile']+='\nfs.o: fs_persistence.inc ata.h\nata.o: ata.h\n\ndisk.img:\n\tpython3 create_disk.py\n'
for name in original:
    shutil.copy2(root/name,root/(name+'.before-persistence'))
for name,text in updated.items():
    (root/name).write_text(text)
print('Applied persistence update. Backups preserved. Run: make clean && make run')
