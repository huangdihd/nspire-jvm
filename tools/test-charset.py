"""Actual charset/buffer bytecode, String conversion and upstream Logback checks."""
import argparse,difflib,os
from pathlib import Path
from test import ROOT,tool,path_for,run
ap=argparse.ArgumentParser();ap.add_argument('--vm',default='build/nspire-jvm');ap.add_argument('--java',required=True,help='Java 8 oracle matching the preserved codecs');ap.add_argument('--only');ap.add_argument('--xinbot',type=Path);ns=ap.parse_args()
java17,javac=tool('java'),tool('javac');java=str(Path(ns.java).resolve());build=ROOT/'build/charset-tests';build.mkdir(parents=True,exist_ok=True)
run([javac,'--release','8','-encoding','UTF-8','-d',path_for(javac,build),*[path_for(javac,p) for p in sorted((ROOT/'charset-tests').glob('*.java'))]])
services=build/'META-INF/services';services.mkdir(parents=True,exist_ok=True)
(services/'java.nio.charset.spi.CharsetProvider').write_text('TestCharsetProvider\n')
report=[]
def check(name,paths,oracle=java):
 sep=';' if oracle.lower().endswith('.exe') else os.pathsep
 expected=run([oracle,'-Dfile.encoding=UTF-8','-cp',sep.join(path_for(oracle,p) for p in paths),name]).stdout
 actual=run([str(Path(ns.vm).resolve()),'--heap','524288','-bootclasspath',ROOT/'dist/runtime.jar.tns','-cp',';'.join(str(p) for p in paths),name]).stdout
 assert expected==actual,name+'\n'+''.join(difflib.unified_diff(expected.splitlines(True),actual.splitlines(True),fromfile='Java',tofile='Nspire JVM'))
 report.append('PASS '+name+' (standard Java oracle)');print(report[-1],flush=True);return actual
for name in ('CharsetTest','EncodingTest','CoderStateTest','BufferTest','WeakReferenceTest','CharsetProviderTest'):
 if not ns.only or ns.only==name:check(name,[build])
if ns.xinbot and (not ns.only or ns.only=='LogbackCharsetTest'):
 xinbot=ns.xinbot.resolve();run([javac,'--release','8','-cp',path_for(javac,xinbot),'-d',path_for(javac,build),*[path_for(javac,p) for p in sorted((ROOT/'charset-tests/logback').glob('*.java'))]])
 output=check('LogbackCharsetTest',[build,xinbot],java17);(ROOT/'LOGBACK-CHARSET-RESULTS.txt').write_text(report[-1]+'\n'+output+'\nComponent check only; whole startup remains incomplete.\n',encoding='utf-8')
 failure=run([str(Path(ns.vm).resolve()),'-bootclasspath',ROOT/'dist/runtime.jar.tns','-cp',str(build)+';'+str(xinbot),'LogbackHeaderPendingTest'],ok=False)
 assert failure.returncode==1 and 'VM error: class not found: java/io/File' in failure.stderr,failure.stderr
 report.append('PASS expected missing File on original Logback header path (sanitizer output checked)');print(report[-1],flush=True)
if not ns.only:(ROOT/'CHARSET-RESULTS.txt').write_text('\n'.join(report)+'\nHost only; calculator execution not verified.\n',encoding='utf-8')
