"""Check JVM termination against Java 8 subprocess status and real disk changes."""
import argparse, os, resource, subprocess, tempfile
from pathlib import Path
from test import ROOT, tool, path_for, run

ap=argparse.ArgumentParser();ap.add_argument('--java',required=True);ap.add_argument('--vm',default='build/nspire-jvm');ap.add_argument('--lifetime',type=Path);ns=ap.parse_args()
java=str(Path(ns.java).resolve());vm=str(Path(ns.vm).resolve());javac=tool('javac')
classes=ROOT/'build/shutdown-tests';classes.mkdir(parents=True,exist_ok=True)
run([javac,'--release','8','-d',path_for(javac,classes),path_for(javac,ROOT/'shutdown-tests/ShutdownTest.java')])
report=[]
def record(text): report.append('PASS '+text);print(report[-1],flush=True)
def snapshot(folder):
    return {p.relative_to(folder).as_posix():p.read_bytes() if p.is_file() else None for p in sorted(folder.rglob('*'))}
def checked(command, folder):
    p=subprocess.run(command,cwd=folder,capture_output=True,text=True,timeout=35)
    for marker in ('ERROR: AddressSanitizer','ERROR: LeakSanitizer','runtime error:'):
        assert marker not in p.stderr,(command,p.stderr)
    return p
prefix=[vm,'--heap','1048576','-bootclasspath',str(ROOT/'dist/runtime.jar.tns'),'-cp',str(classes)]
statuses={'natural':0,'uncaught':1,'exception-hook':0,'late-delete':0,'exit':7,'runtime-exit':11,'worker-exit':9,'halt':23,'worker-halt':29,'halt-hook':31}
for mode,status in statuses.items():
    disks=[]
    for command in ([java,'-cp',str(classes)],prefix):
        with tempfile.TemporaryDirectory(prefix=mode+'-',dir=classes) as temp:
            p=checked([*command,'ShutdownTest',mode],temp)
            assert p.returncode==status,(mode,command,p.returncode,p.stdout,p.stderr)
            disk=snapshot(Path(temp));disks.append(disk)
            assert 'removed-hook' not in disk and 'halt-returned' not in disk,(mode,disk)
            if 'halt' in mode:
                assert disk['delete-me']==b'temporary' and disk['parent/child']==b'temporary',(mode,disk)
                assert 'hook-1' not in disk and 'hook-2' not in disk,(mode,disk)
                if mode=='halt-hook':assert disk['halt-start']==b'yes',disk
            else:
                assert all(name not in disk for name in ('delete-me','parent','parent/child')),(mode,p.stderr,disk)
                assert disk['hook-1']==b'retained=73',(mode,p.stderr,disk)
                if mode!='late-delete':assert disk['hook-2']==b'retained=73',(mode,p.stderr,disk)
            if mode in ('uncaught','exception-hook'):assert 'java.lang.RuntimeException' in p.stderr or 'java/lang/RuntimeException' in p.stderr,p.stderr
    assert disks[0]==disks[1],(mode,disks)
    record(mode+' status, hook execution and disk contents match Linux Java 8')
for mode in ('fatal','fatal-hook','fatal-worker'):
    with tempfile.TemporaryDirectory(prefix=mode+'-',dir=classes) as temp:
        p=checked([*prefix,'ShutdownTest',mode],temp);disk=snapshot(Path(temp))
        assert p.returncode==1 and 'String.format conversion is not implemented' in p.stderr,(mode,p.returncode,p.stdout,p.stderr)
        assert disk['delete-me']==b'temporary' and 'hook-1' not in disk,(mode,disk)
        record(mode+' abort status and no false successful shutdown')
if ns.lifetime:
    def limit_fds():resource.setrlimit(resource.RLIMIT_NOFILE,(64,64))
    with tempfile.TemporaryDirectory(prefix='lifetime-',dir=classes) as temp:
        p=subprocess.run([str(ns.lifetime.resolve()),str(classes),str(ROOT/'dist/runtime.jar.tns')],cwd=temp,capture_output=True,text=True,timeout=120,preexec_fn=limit_fds)
        assert p.returncode==0,(p.stdout,p.stderr)
        for marker in ('ERROR: AddressSanitizer','ERROR: LeakSanitizer','runtime error:'):assert marker not in p.stderr,p.stderr
        assert p.stdout=='PASS twenty embedded VM shutdowns with preserved process and descriptor count\n',p.stdout
        record('twenty embedded shutdowns, exit/halt status, fiber teardown and descriptor count')
(ROOT/'SHUTDOWN-RESULTS.txt').write_text('\n'.join(report)+'\nHost execution only; Ndless hardware, signals, finalizers and ThreadGroup exception dispatch are not verified.\n')
