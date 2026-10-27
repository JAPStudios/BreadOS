from pathlib import Path
import shutil
p=Path(__file__).resolve().parent
for name in ['Makefile','shell.c','shell_parse.c','shell_parse.h','fs.c','fs.h','terminal.h','keyboard.h','shell.h']:
    if not (p/name).is_file():
        raise SystemExit('Missing '+name+'. Use the earlier filesystem source directory.')
m=p/'Makefile';original=m.read_text()
if 'shell_parse.o' in original:
    raise SystemExit('Parser already integrated; no changes made.')
if 'shell.o' not in original:
    raise SystemExit('Unexpected Makefile; no changes made.')
backup=p/'Makefile.before-quoted-shell'
if backup.exists():
    raise SystemExit('Backup already exists; no changes made.')
text=original.replace('shell.o','shell.o shell_parse.o')
text+='\nshell.o shell_parse.o: shell_parse.h\n'
if 'ata.o' in original and 'fs_persistence.inc' in (p/'fs.c').read_text():
    text+='\nCFLAGS += -DFS_PERSISTENCE\n'
shutil.copy2(m,backup)
m.write_text(text)
print('Integrated parser. Run make clean, then make run.')
