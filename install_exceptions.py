from pathlib import Path
import shutil
p=Path(__file__).resolve().parent
for name in ['kernel.c','Makefile','exceptions.c','exceptions.s']:
    if not (p/name).is_file():raise SystemExit('Missing '+name)
k=(p/'kernel.c').read_text();m=(p/'Makefile').read_text()
if 'exceptions_init' in k:raise SystemExit('Already installed; no changes made.')
anchor='void kernel_main(void){'
if anchor not in k or 'boot.o kernel.o' not in m:
    raise SystemExit('Unexpected source layout. This update targets the integrated source bundle; no changes made.')
for name in ['kernel.c','Makefile']:
    if (p/(name+'.before-exceptions')).exists():raise SystemExit('Backup exists; no changes made.')
k='extern void exceptions_init(void);\n'+k.replace(anchor,anchor+'\n exceptions_init();',1)
m=m.replace('boot.o kernel.o','boot.o kernel.o exceptions_c.o exceptions_asm.o')
m+='\nexceptions_c.o: exceptions.c\n\t$(CC) $(CFLAGS) -mgeneral-regs-only -c $< -o $@\n'
m+='\nexceptions_asm.o: exceptions.s\n\t$(CC) -c $< -o $@\n'
for name in ['kernel.c','Makefile']:
    shutil.copy2(p/name,p/(name+'.before-exceptions'))
(p/'kernel.c').write_text(k);(p/'Makefile').write_text(m)
print('Installed exception handling. Run make clean, then make run.')
