"""Build and exercise the integrated starter in QEMU; never uses disk.img."""
import argparse,json,os,shutil,socket,subprocess,tempfile,time
from pathlib import Path

def decode_vga(raw):
    if len(raw)!=4000:raise ValueError('Expected 4000-byte VGA buffer')
    return '\n'.join(raw[r*160:r*160+160:2].decode('ascii',errors='replace').rstrip() for r in range(25))

def qcode(c):
    if c in 'abcdefghijklmnopqrstuvwxyz0123456789':return c
    return {' ':'spc','/':'slash','.':'dot','\n':'ret','-':'minus'}.get(c)

class Machine:
    def __init__(self,iso,disk,work):
        self.work=Path(work);self.socket_path=self.work/'qmp.sock'
        self.socket_path.unlink(missing_ok=True)
        self.log=(self.work/'qemu.log').open('wb')
        self.process=subprocess.Popen(['qemu-system-i386','-m','64','-boot','d','-cdrom',str(iso),
            '-drive',f'file={disk},format=raw,if=ide,index=0,media=disk',
            '-display','none','-serial','none','-monitor','none','-no-reboot','-no-shutdown',
            '-qmp',f'unix:{self.socket_path},server=on,wait=off'],stdout=self.log,stderr=subprocess.STDOUT)
        self.sock=None;self.stream=None;self.serial=0
        try:
            deadline=time.monotonic()+15
            while time.monotonic()<deadline:
                if self.process.poll() is not None:raise RuntimeError('QEMU exited; inspect qemu.log')
                sock=socket.socket(socket.AF_UNIX,socket.SOCK_STREAM);sock.settimeout(3)
                try:sock.connect(str(self.socket_path));self.sock=sock;break
                except OSError:sock.close();time.sleep(.1)
            if self.sock is None:raise TimeoutError('QMP socket did not appear')
            self.stream=self.sock.makefile('rb')
            greeting=json.loads(self.stream.readline())
            if 'QMP' not in greeting:raise RuntimeError('Invalid QMP greeting')
            self.call('qmp_capabilities')
        except Exception:
            self.close();raise
    def call(self,command,arguments=None):
        self.serial+=1;ident=self.serial
        request={'execute':command,'id':ident}
        if arguments is not None:request['arguments']=arguments
        self.sock.sendall(json.dumps(request).encode()+b'\n')
        while True:
            raw=self.stream.readline()
            if not raw:raise RuntimeError('QMP connection closed')
            response=json.loads(raw)
            if response.get('id')!=ident:continue
            if 'error' in response:raise RuntimeError(str(response['error']))
            return response.get('return')
    def screen(self):
        target=self.work/'vga.bin'
        self.call('pmemsave',{'val':0xB8000,'size':4000,'filename':str(target)})
        return decode_vga(target.read_bytes())
    def wait(self,predicate,seconds=20):
        end=time.monotonic()+seconds;last=''
        while time.monotonic()<end:
            last=self.screen()
            if predicate(last):return last
            time.sleep(.2)
        raise TimeoutError('Screen condition not met. Last screen:\n'+last)
    def command(self,line):
        for c in line+'\n':
            key=qcode(c)
            if key is None:raise ValueError('Unsupported test input character: '+repr(c))
            self.call('send-key',{'keys':[{'type':'qcode','data':key}],'hold-time':20})
            time.sleep(.07)
        time.sleep(.3)
    def close(self):
        if getattr(self,'process',None) and self.process.poll() is None:
            self.process.terminate()
            try:self.process.wait(timeout=5)
            except subprocess.TimeoutExpired:self.process.kill();self.process.wait()
        if self.stream:self.stream.close()
        if self.sock:self.sock.close()
        if getattr(self,'log',None):self.log.close()

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--project',default='.')
    parser.add_argument('--skip-build',action='store_true')
    args=parser.parse_args();project=Path(args.project).resolve()
    report=project/'BOOT-TEST-RESULTS.json'
    result={'status':'NOT RUN','checks':[],'scope':'integrated starter only'}
    machine=None
    try:
        if not hasattr(socket,'AF_UNIX') or os.name!='posix':raise RuntimeError('Requires Linux/macOS POSIX build host with Unix sockets')
        if not shutil.which('qemu-system-i386'):raise RuntimeError('Missing qemu-system-i386')
        if not args.skip_build:
            for tool in ['make','i686-elf-gcc','grub-file','grub-mkrescue','xorriso']:
                if not shutil.which(tool):raise RuntimeError('Missing build prerequisite: '+tool)
            # Fresh objects avoid accidentally reporting a stale source build.
            with (project/'BOOT-BUILD.log').open('w') as log:
                subprocess.run(['make','clean'],cwd=project,stdout=log,stderr=subprocess.STDOUT,check=True)
                subprocess.run(['make','iso'],cwd=project,stdout=log,stderr=subprocess.STDOUT,check=True)
        iso=project/'starter.iso'
        if not iso.is_file():raise RuntimeError('Missing starter.iso')
        with tempfile.TemporaryDirectory(prefix='os-boot-test-') as tmp:
            work=Path(tmp);disk=work/'test.img'
            with disk.open('xb') as handle:handle.truncate(1024*1024)
            machine=Machine(iso,disk,work)
            machine.wait(lambda s:'Kernel ready.' in s and '> ' in s)
            result['checks'].append('Kernel banner and prompt observed in emulated VGA memory')
            for command in ['mkdir /test','touch /probe.txt','write /probe.txt content123','mv /probe.txt /test/','clear','cat /test/probe.txt']:
                machine.command(command)
            machine.wait(lambda s:'content123' in s.splitlines())
            result['checks'].append('File content read after move into directory')
            for command in ['mv /test/probe.txt /','clear','cat /probe.txt']:
                machine.command(command)
            machine.wait(lambda s:'content123' in s.splitlines())
            result['checks'].append('File content read after move out of directory')
            machine.command('save')
            machine.wait(lambda s:'Saved to disk.img.' in s.splitlines())
            # Guest message says disk.img; actual test disk is isolated test.img.
            machine.close();machine=None
            machine=Machine(iso,disk,work)
            machine.wait(lambda s:'Kernel ready.' in s)
            machine.command('load')
            machine.wait(lambda s:'Loaded snapshot; current directory /.' in s.splitlines())
            machine.command('clear');machine.command('cat /probe.txt')
            screen=machine.wait(lambda s:'content123' in s.splitlines())
            result['checks'].append('Saved contents restored after a fresh QEMU process')
            result['last_screen']=screen
            machine.close();machine=None
        result['status']='PASS'
    except Exception as error:
        result['status']='FAIL_OR_NOT_RUN';result['error']=str(error)
        if machine:
            try:result['last_screen']=machine.screen()
            except Exception:pass
    finally:
        if machine:machine.close()
        report.write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result,indent=2))
    raise SystemExit(0 if result['status']=='PASS' else 1)
if __name__=='__main__':main()
