"""Java native linkage semantics; these checks do not claim working JNI loading."""
import argparse,os,subprocess,tempfile
from pathlib import Path
from test import ROOT,tool,path_for,run
ap=argparse.ArgumentParser();ap.add_argument('--vm',default='build/nspire-jvm');ap.add_argument('--java',required=True);ns=ap.parse_args()
java=str(Path(ns.java).resolve());vm=str(Path(ns.vm).resolve());javac=tool('javac')
if java.endswith('.exe'):ap.error('Library naming oracle must be Linux Java')
classes=ROOT/'build/native-tests';classes.mkdir(parents=True,exist_ok=True)
run([javac,'--release','8','-encoding','UTF-8','-d',path_for(javac,classes),*[path_for(javac,p) for p in sorted((ROOT/'native-tests').glob('*.java'))]])
report=[]
with tempfile.TemporaryDirectory(prefix='native-linkage-',dir=classes) as tmp:
    folder=Path(tmp);(folder/'invalid.so').write_bytes(b'not an ELF shared library\n')
    tempdir=folder/'临时😀';tempdir.mkdir()
    for name in ('LibraryNameTest','NativeLoadTest','NativeLinkageTest','NativeThreadExceptionTest','TempDirectoryTest','UnsignedTextTest'):
        outputs=[]
        for base in ([java,'-Djava.io.tmpdir='+str(tempdir),'-cp',str(classes)],[vm,'--tmpdir',str(tempdir),'--heap','65536','-bootclasspath',str(ROOT/'dist/runtime.jar.tns'),'-cp',str(classes)]):
            p=subprocess.run([*base,name],cwd=folder,capture_output=True,text=True,timeout=60)
            assert p.returncode==0,(base,name,p.returncode,p.stdout,p.stderr)
            for marker in ('ERROR: AddressSanitizer','ERROR: LeakSanitizer','runtime error:'):assert marker not in p.stderr,p.stderr
            if name=='NativeThreadExceptionTest':assert 'java/lang/UnsatisfiedLinkError' in p.stderr.replace('.','/'),p.stderr
            outputs.append(p.stdout)
        assert outputs[0]==outputs[1],(name,outputs)
        report.append('PASS '+name+' (Linux Java oracle; VM heap 64 KiB)');print(report[-1],flush=True)
    assert not list(tempdir.iterdir()),'Temporary probe file was not removed'
(ROOT/'NATIVE-RESULTS.txt').write_text('\n'.join(report)+'\nNative library failure semantics only; dynamic JNI loading and device execution are not verified.\n')
