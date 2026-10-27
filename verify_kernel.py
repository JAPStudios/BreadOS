import argparse,json,struct
from pathlib import Path

def verify(path):
    data=Path(path).read_bytes()
    if len(data)<52 or data[:7]!=b'\x7fELF\x01\x01\x01':raise ValueError('Not little-endian ELF32')
    header=struct.unpack_from('<16sHHIIIIIHHHHHH',data)
    _,kind,machine,version,entry,phoff,_,_,ehsize,phsize,phnum,_,_,_=header
    if kind!=2 or machine!=3 or version!=1 or ehsize!=52:raise ValueError('Not a 32-bit x86 executable')
    if phsize!=32 or phnum==0 or phoff+phsize*phnum>len(data):raise ValueError('Invalid program headers')
    executable=False;loads=[]
    for i in range(phnum):
        typ,offset,vaddr,paddr,filesz,memsz,flags,align=struct.unpack_from('<IIIIIIII',data,phoff+i*32)
        if typ in (2,3):raise ValueError('Dynamic loader/dependency segment is not supported')
        if typ==1:
            if filesz>memsz or offset+filesz>len(data):raise ValueError('Invalid load segment')
            loads.append((offset,filesz))
            if flags&1 and vaddr<=entry<vaddr+filesz:executable=True
    if not executable:raise ValueError('Entry point is not in file-backed executable code')
    found=None
    for offset in range(0,min(8192,len(data))-11,4):
        magic,flags,check=struct.unpack_from('<III',data,offset)
        if magic!=0x1BADB002 or (magic+flags+check)&0xffffffff:continue
        # This validator intentionally accepts only this starter's ELF header form.
        if flags!=3:continue
        if not any(start<=offset and offset+12<=start+size for start,size in loads):continue
        found=offset;break
    if found is None:raise ValueError('Starter Multiboot header missing/invalid/not loaded')
    return {'result':'STRUCTURE_PASS','entry':hex(entry),'multiboot_header_offset':found,'bytes':len(data),'runtime_verified':False}
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('kernel',nargs='?',default='kernel.elf');a=p.parse_args()
    try:print(json.dumps(verify(a.kernel),indent=2))
    except (ValueError,OSError,struct.error) as e:p.exit(1,'Verification failed: '+str(e)+'\n')
