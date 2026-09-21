"""Real NIO paths, stream copies and seekable files against Linux Java 8."""
import argparse, hashlib, os, resource, subprocess, tempfile
from pathlib import Path
from test import ROOT, tool, path_for, run

ap=argparse.ArgumentParser();ap.add_argument('--java',required=True);ap.add_argument('--vm',default='build/nspire-jvm');ap.add_argument('--lifetime',type=Path);ns=ap.parse_args()
java=str(Path(ns.java).resolve());vm=str(Path(ns.vm).resolve());javac=tool('javac')
if java.endswith('.exe'):ap.error('Use Linux Java to compare Unix file semantics')
classes=ROOT/'build/nio-file-tests';classes.mkdir(parents=True,exist_ok=True)
run([javac,'--release','8','-encoding','UTF-8','-d',path_for(javac,classes),*[path_for(javac,p) for p in sorted((ROOT/'nio-file-tests').glob('*.java'))]])
def prepare(folder):
    (folder/'original').write_bytes(b'original unchanged\n')
    (folder/'emptydir').mkdir();(folder/'nonempty').mkdir();(folder/'nonempty/child').write_bytes(b'child\n')
    (folder/'link').symlink_to('original');(folder/'symlink').symlink_to('original');(folder/'dangling').symlink_to('absent')
    os.link(folder/'original',folder/'hardlink')
def snapshot(folder):
    result={}
    for path in sorted(folder.rglob('*')):
        name=path.relative_to(folder).as_posix()
        result[name]=('link',os.readlink(path)) if path.is_symlink() else ('file',hashlib.sha256(path.read_bytes()).hexdigest()) if path.is_file() else ('directory',)
    return result
report=[]
for name in ('NioPathTest','NioCopyTest','NioChannelTest'):
    outputs=[];contents=[]
    for command in ([java,'-cp',str(classes)],[vm,'--heap','2097152','-bootclasspath',str(ROOT/'dist/runtime.jar.tns'),'-cp',str(classes)]):
        with tempfile.TemporaryDirectory(prefix='nio-',dir=classes) as temp:
            folder=Path(temp);prepare(folder)
            p=subprocess.run([*command,name],cwd=folder,capture_output=True,text=True,timeout=120)
            assert p.returncode==0,(command,name,p.returncode,p.stdout[-3000:],p.stderr)
            for marker in ('ERROR: AddressSanitizer','ERROR: LeakSanitizer','runtime error:'):assert marker not in p.stderr,p.stderr
            outputs.append(p.stdout);contents.append(snapshot(folder))
    if outputs[0]!=outputs[1]:
        (classes/(name+'-java.txt')).write_text(outputs[0]);(classes/(name+'-vm.txt')).write_text(outputs[1])
        import difflib
        raise AssertionError(name+'\n'+''.join(difflib.unified_diff(outputs[0].splitlines(True),outputs[1].splitlines(True))))
    assert contents[0]==contents[1],(name,contents)
    report.append('PASS '+name+' (Linux Java 8 output and actual disk contents)');print(report[-1],flush=True)
def limit_fds(): resource.setrlimit(resource.RLIMIT_NOFILE,(64,64))
for fatal in (False,True):
    with tempfile.TemporaryDirectory(prefix='nio-gc-',dir=classes) as temp:
        folder=Path(temp);prepare(folder)
        p=subprocess.run([vm,'--heap','1048576','-bootclasspath',str(ROOT/'dist/runtime.jar.tns'),'-cp',str(classes),'NioGcTest',*(['fatal'] if fatal else [])],cwd=folder,capture_output=True,text=True,timeout=90,preexec_fn=limit_fds)
        assert p.returncode==int(fatal),(p.returncode,p.stdout,p.stderr)
        assert p.stdout=='200\n' and (folder/'gc-output').read_bytes()==bytes([7])*200,(p.stdout,p.stderr)
        assert (folder/'exit-output').read_bytes()==bytes([11])
        for marker in ('ERROR: AddressSanitizer','ERROR: LeakSanitizer','runtime error:'):assert marker not in p.stderr,p.stderr
        if fatal: assert 'String.format conversion is not implemented' in p.stderr,p.stderr
        report.append('PASS NIO descriptor GC under 64-handle limit'+(' and fatal exit' if fatal else ''));print(report[-1],flush=True)
if ns.lifetime:
    # Compile the existing console-close probe for the shared embedding harness.
    source=ROOT/'file-tests/ConsoleCloseTest.java'
    run([javac,'--release','8','-d',path_for(javac,classes),path_for(javac,source)])
    with tempfile.TemporaryDirectory(prefix='nio-lifetime-',dir=classes) as temp:
        folder=Path(temp);prepare(folder)
        p=subprocess.run([str(ns.lifetime.resolve()),str(classes),str(ROOT/'dist/runtime.jar.tns'),'NioGcTest'],cwd=folder,capture_output=True,text=True,timeout=180,preexec_fn=limit_fds)
        assert p.returncode==0,(p.stdout,p.stderr)
        assert p.stdout.endswith('PASS descriptors unchanged across six normal/fatal VM runs\n'),p.stdout
        assert (folder/'gc-output').read_bytes()==bytes([7])*1200
        for marker in ('ERROR: AddressSanitizer','ERROR: LeakSanitizer','runtime error:'):assert marker not in p.stderr,p.stderr
        report.append('PASS NIO descriptor counts across six normal/fatal VM runs and console close');print(report[-1],flush=True)
(ROOT/'NIO-FILE-RESULTS.txt').write_text('\n'.join(report)+'\nHost filesystem only; Ndless execution and unsupported operations are not verified.\n')
