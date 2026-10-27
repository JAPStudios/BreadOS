"""Offline host tool for the integrated starter's BSF2 disk snapshot only."""
import argparse
import os
from pathlib import Path
import shutil
import struct
import tempfile
import uuid
N=64
RECORD=struct.Struct('<III32s1024s')
SIZE=N*RECORD.size
MAGIC=0x32465342

def checksum(data):
    h=2166136261
    for byte in data:
        h=((h^byte)*16777619)&0xffffffff
    return h

def field(raw):
    if b'\0' not in raw:
        raise ValueError('Unterminated snapshot field')
    return raw.split(b'\0',1)[0]

class Store:
    def __init__(self,image):
        self.path=Path(image)
        self.original=self.path.read_bytes()
        if len(self.original)<1024*1024 or len(self.original)>16*1024*1024:
            raise ValueError('Expected dedicated 1-16 MiB emulator image')
        magic,size,expected=struct.unpack_from('<III',self.original)
        raw=self.original[512:512+SIZE]
        if magic!=MAGIC or size!=SIZE or checksum(raw)!=expected:
            raise ValueError('Not a valid integrated-starter snapshot. Run save inside that kernel first.')
        self.nodes=[]
        for i in range(N):
            used,directory,parent,name,data=RECORD.unpack_from(raw,i*RECORD.size)
            self.nodes.append(dict(used=used,dir=directory,parent=parent,name=field(name),data=field(data)))
        self.validate()
    def validate(self):
        root=self.nodes[0]
        if (root['used'],root['dir'],root['parent'],root['name'])!=(1,1,0,b''):
            raise ValueError('Invalid root')
        seen=set()
        for i,n in enumerate(self.nodes[1:],1):
            if n['used'] not in (0,1): raise ValueError('Invalid used flag')
            if not n['used']: continue
            if n['dir'] not in (0,1) or not 0<=n['parent']<N:raise ValueError('Invalid node')
            p=self.nodes[n['parent']]
            if not p['used'] or not p['dir']:raise ValueError('Invalid parent')
            if not n['name'] or n['name'] in (b'.',b'..') or b'/' in n['name']:raise ValueError('Invalid name')
            key=(n['parent'],n['name'])
            if key in seen:raise ValueError('Duplicate name')
            seen.add(key)
            at=i;visited=set()
            while at:
                if at in visited or not 0<=at<N or not self.nodes[at]['used']:raise ValueError('Directory cycle')
                visited.add(at);at=self.nodes[at]['parent']
    def child(self,parent,name):
        for i,n in enumerate(self.nodes[1:],1):
            if n['used'] and n['parent']==parent and n['name']==name:return i
        return None
    def resolve(self,path):
        if not path or not path.startswith('/'):raise ValueError('Use absolute image paths beginning with /')
        at=0
        for part in path.split('/'):
            if not part:continue
            if not self.nodes[at]['dir']:raise ValueError('Path traverses a file')
            if part=='.':continue
            if part=='..':at=self.nodes[at]['parent'];continue
            at=self.child(at,part.encode('ascii'))
            if at is None:raise ValueError('Image path not found: '+path)
        if path.endswith('/') and not self.nodes[at]['dir']:raise ValueError('Not a directory')
        return at
    def destination(self,path):
        if not path.startswith('/') or path.endswith('/'):raise ValueError('Use a full absolute destination filename')
        parent,_,name=path.rpartition('/')
        b=name.encode('ascii')
        if not b or len(b)>31 or b'\0' in b or name in ('.','..'):raise ValueError('Invalid or overlong name')
        p=self.resolve(parent or '/')
        if not self.nodes[p]['dir']:raise ValueError('Destination parent is not a folder')
        if self.child(p,b) is not None:raise ValueError('Destination exists; refusing overwrite')
        return p,b
    def create(self,path,directory,data=b''):
        p,name=self.destination(path)
        if len(data)>1023 or b'\0' in data:raise ValueError('Only NUL-free text up to 1023 bytes is supported')
        data.decode('ascii')
        for i in range(1,N):
            if not self.nodes[i]['used']:
                self.nodes[i]=dict(used=1,dir=int(directory),parent=p,name=name,data=data)
                return i
        raise ValueError('Image filesystem full')
    def move(self,source,destination):
        i=self.resolve(source)
        if i==0:raise ValueError('Cannot move root')
        try:d=self.resolve(destination)
        except ValueError:d=None
        if d is not None:
            if not self.nodes[d]['dir']:raise ValueError('Destination exists')
            p=d;name=self.nodes[i]['name']
            if self.child(p,name) is not None:raise ValueError('Destination exists')
        else:p,name=self.destination(destination)
        at=p
        while True:
            if at==i:raise ValueError('Cannot move directory inside itself')
            if at==0:break
            at=self.nodes[at]['parent']
        self.nodes[i]['parent']=p;self.nodes[i]['name']=name
    def commit(self):
        self.validate()
        raw=b''.join(RECORD.pack(n['used'],n['dir'],n['parent'],n['name'],n['data']) for n in self.nodes)
        image=bytearray(self.original)
        image[:512]=bytes(512)
        struct.pack_into('<III',image,0,MAGIC,SIZE,checksum(raw))
        image[512:512+SIZE]=raw
        if self.path.read_bytes()!=self.original:raise ValueError('Image changed during edit; refusing write')
        backup=self.path.with_name(self.path.name+'.backup-'+uuid.uuid4().hex)
        with backup.open('xb') as b:
            b.write(self.original);b.flush();os.fsync(b.fileno())
        fd,temp=tempfile.mkstemp(prefix=self.path.name+'.tmp-',dir=self.path.parent)
        try:
            with os.fdopen(fd,'wb') as handle:
                handle.write(image);handle.flush();os.fsync(handle.fileno())
            if self.path.read_bytes()!=self.original:raise ValueError('Image changed during edit; refusing replacement')
            os.replace(temp,self.path)
        finally:
            if os.path.exists(temp):os.unlink(temp)
        self.original=bytes(image)
        return backup

def main():
    p=argparse.ArgumentParser(description=__doc__+' CLOSE QEMU BEFORE USE.')
    p.add_argument('--image',default='disk.img')
    p.add_argument('--offline',action='store_true',help='Confirm QEMU is closed; required for every operation')
    sub=p.add_subparsers(dest='command',required=True)
    a=sub.add_parser('ls');a.add_argument('path',nargs='?',default='/')
    a=sub.add_parser('mkdir');a.add_argument('path')
    a=sub.add_parser('import');a.add_argument('host_file');a.add_argument('image_path')
    a=sub.add_parser('export');a.add_argument('image_path');a.add_argument('host_file')
    a=sub.add_parser('mv');a.add_argument('source');a.add_argument('destination')
    args=p.parse_args()
    if not args.offline:p.error('Close QEMU, then pass --offline. Concurrent access is not supported.')
    try:
        store=Store(args.image)
        if args.command=='ls':
            i=store.resolve(args.path)
            if not store.nodes[i]['dir']:raise ValueError('Not a directory')
            for n in store.nodes[1:]:
                if n['used'] and n['parent']==i:print(n['name'].decode('ascii')+('/' if n['dir'] else ''))
        elif args.command=='export':
            n=store.nodes[store.resolve(args.image_path)]
            if n['dir']:raise ValueError('Cannot export a directory')
            with Path(args.host_file).open('xb') as dest:dest.write(n['data'])
            print('Exported; image unchanged.')
        else:
            if args.command=='mkdir':store.create(args.path,True)
            elif args.command=='import':
                with Path(args.host_file).open('rb') as src:data=src.read(1024)
                store.create(args.image_path,False,data)
            else:store.move(args.source,args.destination)
            backup=store.commit()
            print('Updated image. Backup:',backup)
    except (OSError,ValueError,UnicodeError,struct.error) as e:
        p.exit(1,'Error: '+str(e)+'\n')
if __name__=='__main__':main()
