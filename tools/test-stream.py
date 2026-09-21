"""Execute unmodified OpenJDK stream/enum library code and compare with Java."""
import argparse,difflib,zipfile
from pathlib import Path
from test import ROOT,tool,path_for,run

ap=argparse.ArgumentParser();ap.add_argument('--vm',default='build/nspire-jvm');ap.add_argument('--only');ns=ap.parse_args()
java,javac=tool('java'),tool('javac');build=ROOT/'build/stream-tests';build.mkdir(parents=True,exist_ok=True)
run([javac,'--release','8','-encoding','UTF-8','-d',path_for(javac,build),*[path_for(javac,p) for p in sorted((ROOT/'stream-tests').glob('*.java'))]])
jar=build/'streams.jar'
with zipfile.ZipFile(jar,'w',zipfile.ZIP_DEFLATED) as z:
    for p in build.glob('*.class'):z.write(p,p.name)
report=[]
for name in ('EnumTest','StreamSupportTest','WideBoxingTest','StreamTest'):
    if ns.only and ns.only!=name:continue
    expected=run([java,'-cp',path_for(java,jar),name]).stdout
    result=run([str(Path(ns.vm).resolve()),'--heap','262144','-bootclasspath',ROOT/'dist/runtime.jar.tns','-cp',jar,name])
    assert result.stdout==expected,''.join(difflib.unified_diff(expected.splitlines(True),result.stdout.splitlines(True),fromfile='Java',tofile='Nspire JVM'))
    report.append('PASS '+name+' (actual OpenJDK library bytecode)');print(report[-1],flush=True)
if not ns.only:
    for name,message in [('UnsupportedParallelTest','class not found: java/util/concurrent/CountedCompleter'),('UnsupportedDoubleTextTest','Double object text formatting is not implemented')]:
        result=run([str(Path(ns.vm).resolve()),'-bootclasspath',ROOT/'dist/runtime.jar.tns','-cp',jar,name],ok=False)
        assert result.returncode==1 and message in result.stderr,(name,result.stdout,result.stderr)
        report.append('PASS explicit failure '+name+' (sanitizer output checked)');print(report[-1],flush=True)
    (ROOT/'STREAM-RESULTS.txt').write_text('\n'.join(report)+'\nHost only; calculator execution not verified.\n')
