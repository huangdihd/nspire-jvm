"""Original time arithmetic, TZDB rules and the unchanged Xinbot date formatter."""
import argparse,difflib,os
from pathlib import Path
from test import ROOT,tool,path_for,run
ap=argparse.ArgumentParser();ap.add_argument('--vm',default='build/nspire-jvm');ap.add_argument('--java',required=True);ap.add_argument('--java17');ap.add_argument('--xinbot',type=Path);ap.add_argument('--only');ns=ap.parse_args()
java=str(Path(ns.java).resolve());javac=tool('javac');build=ROOT/'build/time-tests';build.mkdir(parents=True,exist_ok=True)
run([javac,'--release','8','-encoding','UTF-8','-d',path_for(javac,build),*[path_for(javac,p) for p in sorted((ROOT/'time-tests').glob('*.java'))]])
report=[]
def check(name,paths,oracle=java):
    separator=';' if oracle.lower().endswith('.exe') else os.pathsep
    expected=run([oracle,'-Duser.timezone=Asia/Hong_Kong','-cp',separator.join(path_for(oracle,p) for p in paths),name],timeout=120).stdout
    result=run([str(Path(ns.vm).resolve()),'--timezone','Asia/Hong_Kong','-bootclasspath',ROOT/'dist/runtime.jar.tns','-cp',';'.join(map(str,paths)),name],timeout=120)
    assert result.stdout==expected,name+'\n'+''.join(difflib.unified_diff(expected.splitlines(True),result.stdout.splitlines(True),fromfile='Java',tofile='Nspire JVM'))
    report.append('PASS '+name+' (standard Java oracle)');print(report[-1],flush=True);return result.stdout
for name in ('TimeSupportTest','TimeTest'):
    if not ns.only or ns.only==name:check(name,[build])
if ns.xinbot and (not ns.only or ns.only=='LogbackDateTest'):
    xinbot=ns.xinbot.resolve();oracle=str(Path(ns.java17).resolve()) if ns.java17 else tool('java')
    run([javac,'--release','8','-cp',path_for(javac,xinbot),'-d',path_for(javac,build),path_for(javac,ROOT/'time-tests/logback/LogbackDateTest.java')])
    output=check('LogbackDateTest',[build,xinbot],oracle)
    (ROOT/'LOGBACK-DATE-RESULTS.txt').write_text(report[-1]+'\n'+output+'\nComponent check only; whole startup remains incomplete.\n')
if not ns.only:(ROOT/'TIME-RESULTS.txt').write_text('\n'.join(report)+'\nHost only; calculator execution is unverified.\n')
