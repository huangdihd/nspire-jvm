"""Actual unmodified Xinbot Jansi extraction, failed load and JVM exit cleanup."""
import argparse, hashlib, subprocess, tempfile, zipfile
from pathlib import Path
from test import ROOT, tool, path_for, run
ap=argparse.ArgumentParser();ap.add_argument('--xinbot',required=True,type=Path);ap.add_argument('--vm',default='build/nspire-jvm');ns=ap.parse_args()
jar=ns.xinbot.resolve();assert hashlib.sha256(jar.read_bytes()).hexdigest()=='601a870da7a4a617172c9fe145ea8bcbe8f7648deddd681e2c96d749cd14f853'
with zipfile.ZipFile(jar) as z:expected=z.read('org/fusesource/jansi/internal/native/Linux/x86_64/libjansi.so')
classes=ROOT/'build/jansi-cleanup';classes.mkdir(parents=True,exist_ok=True);javac=tool('javac')
run([javac,'--release','8','-d',path_for(javac,classes),path_for(javac,ROOT/'shutdown-tests/jansi/JansiCleanupTest.java')])
report=[]
for explicit in (False,True):
    with tempfile.TemporaryDirectory(prefix='jansi-cleanup-',dir=classes) as temp:
        folder=Path(temp);tmp=folder/'tmp';tmp.mkdir()
        p=subprocess.run([str(Path(ns.vm).resolve()),'--tmpdir',str(tmp),'-bootclasspath',str(ROOT/'dist/runtime.jar.tns'),'-cp',str(classes)+';'+str(jar),'JansiCleanupTest',*(['exit'] if explicit else [])],cwd=folder,capture_output=True,text=True,timeout=60)
        assert p.returncode==(17 if explicit else 0),(p.returncode,p.stdout,p.stderr)
        assert 'Dynamic JNI library loading is not available in Nspire JVM' in p.stderr,p.stderr
        for marker in ('ERROR: AddressSanitizer','ERROR: LeakSanitizer','runtime error:','VM error:','Uncaught Java exception'):assert marker not in p.stderr,p.stderr
        assert (folder/'captured-library').read_bytes()==expected
        assert (folder/'hook-ran').read_bytes()==bytes([73])
        assert list(tmp.iterdir())==[],list(tmp.iterdir())
        report.append('PASS original Jansi extraction, exact library bytes, caught JNI failure, hook-before-deletion and '+('System.exit(17)' if explicit else 'natural shutdown'));print(report[-1],flush=True)
(ROOT/'JANSI-CLEANUP-RESULTS.txt').write_text('\n'.join(report)+'\nHost component check only; native loading, full Xinbot startup and Ndless execution are not verified.\n')
