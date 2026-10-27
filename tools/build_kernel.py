"""Build integrated starter using an available GCC; no host runtime libraries."""
import argparse,json,shutil,subprocess,sys
from pathlib import Path
from verify_kernel import verify

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--project',default='.')
    p.add_argument('--cc',default='i686-elf-gcc' if shutil.which('i686-elf-gcc') else 'gcc')
    p.add_argument('--iso',action='store_true')
    a=p.parse_args();root=Path(a.project).resolve();build=root/'build-checked';build.mkdir(exist_ok=True)
    result={'status':'NOT_BUILT','runtime_verified':False,'compiler':a.cc}
    try:
        if not shutil.which(a.cc):raise RuntimeError('Compiler not found: '+a.cc)
        for name in ['boot.s','kernel.c','linker.ld']:
            if not (root/name).is_file():raise RuntimeError('Missing '+name)
        source=(root/'kernel.c').read_text()
        modules=[('boot.s','boot.o'),('kernel.c','kernel.o')]
        if '#include "heap.h"' in source:modules.append(('heap.c','heap.o'))
        if 'exceptions_init();' in source:modules.extend([('exceptions.c','exceptions_c.o'),('exceptions.s','exceptions_asm.o')])
        flags=['-m32','-march=i686','-ffreestanding','-fno-builtin','-fno-pie','-fno-stack-protector','-fno-asynchronous-unwind-tables','-mno-sse','-mno-mmx','-msoft-float','-O2','-Wall','-Wextra']
        with (build/'build.log').open('w') as log:
            for name,obj in modules:
                if not (root/name).is_file():raise RuntimeError('Integrated module missing: '+name)
                subprocess.run([a.cc,*flags,'-std=gnu11','-c',str(root/name),'-o',str(build/obj)],stdout=log,stderr=subprocess.STDOUT,check=True)
            target=build/'kernel.elf'
            # No libgcc is linked. Any unresolved compiler helper is a hard failure.
            subprocess.run([a.cc,'-m32','-nostdlib','-static','-no-pie','-Wl,--build-id=none','-Wl,-T,'+str(root/'linker.ld'),'-o',str(target),*[str(build/obj) for _,obj in modules]],stdout=log,stderr=subprocess.STDOUT,check=True)
            result.update(verify(target));result['status']='KERNEL_BUILT_STRUCTURE_CHECKED'
            if a.iso:
                for tool in ['grub-file','grub-mkrescue','xorriso']:
                    if not shutil.which(tool):raise RuntimeError('Missing ISO tool: '+tool)
                if not (root/'grub.cfg').is_file():raise RuntimeError('Missing grub.cfg')
                subprocess.run(['grub-file','--is-x86-multiboot',str(target)],stdout=log,stderr=subprocess.STDOUT,check=True)
                tree=build/'iso'/'boot'/'grub';tree.mkdir(parents=True,exist_ok=True)
                shutil.copy2(root/'grub.cfg',tree/'grub.cfg');shutil.copy2(target,tree.parent/'kernel.elf')
                subprocess.run(['grub-mkrescue','-o',str(build/'starter.iso'),str(build/'iso')],stdout=log,stderr=subprocess.STDOUT,check=True)
                result['status']='ISO_BUILT_NOT_BOOT_TESTED'
    except Exception as error:
        result['status']='FAILED';result['error']=str(error)
    (build/'BUILD-RESULTS.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result,indent=2))
    return 1 if result['status']=='FAILED' else 0
if __name__=='__main__':raise SystemExit(main())
