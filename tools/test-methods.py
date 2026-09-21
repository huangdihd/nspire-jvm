"""Method discovery/invocation comparisons and actual Logback bean discovery."""
import argparse,difflib,os,zipfile
from pathlib import Path
from test import ROOT,tool,path_for,run
ap=argparse.ArgumentParser();ap.add_argument('--vm',default='build/nspire-jvm');ap.add_argument('--only');ap.add_argument('--xinbot',type=Path);ns=ap.parse_args()
java,javac=tool('java'),tool('javac');build=ROOT/'build/method-tests';build.mkdir(parents=True,exist_ok=True)
run([javac,'--release','8','-encoding','UTF-8','-d',path_for(javac,build),*[path_for(javac,p) for p in sorted((ROOT/'method-tests').rglob('*.java')) if 'logback' not in p.parts]])
report=[]
def check(name,paths):
    sep=';' if java.lower().endswith('.exe') else os.pathsep
    expected=run([java,'-Dfile.encoding=UTF-8','-cp',sep.join(path_for(java,p) for p in paths),name]).stdout
    actual=run([str(Path(ns.vm).resolve()),'--heap','262144','-bootclasspath',ROOT/'dist/runtime.jar.tns','-cp',';'.join(str(p) for p in paths),name]).stdout
    assert actual==expected,name+'\n'+''.join(difflib.unified_diff(expected.splitlines(True),actual.splitlines(True),fromfile='Java',tofile='Nspire JVM'))
    report.append('PASS '+name+' (standard Java oracle)');print(report[-1],flush=True);return actual
for name in ('MethodDiscoveryTest','MethodInvokeTest','MethodAnnotationTest','MethodAccessTest','MethodInitializationTest','MethodConversionTest','PackageTest','InterfacesTest'):
    if not ns.only or ns.only==name:check(name,[build])
if ns.xinbot and (not ns.only or ns.only=='LogbackBeanTest'):
    xinbot=ns.xinbot.resolve()
    run([javac,'--release','8','-cp',path_for(javac,xinbot),'-d',path_for(javac,build),path_for(javac,ROOT/'method-tests/logback/LogbackBeanTest.java')])
    result=check('LogbackBeanTest',[build,xinbot])
    (ROOT/'LOGBACK-BEAN-RESULTS.txt').write_text(report[-1]+'\n'+result+'\nComponent check only; whole Xinbot startup is not verified.\n',encoding='utf-8')
if not ns.only:
    failure=run([str(Path(ns.vm).resolve()),'-bootclasspath',ROOT/'dist/runtime.jar.tns','-cp',build,'UnsupportedMethodTest'],ok=False)
    assert failure.returncode==1 and 'runtime method not implemented: java/lang/String.contentEquals(Ljava/lang/StringBuffer;)Z' in failure.stderr,failure.stderr
    report.append('PASS explicit unsupported reflected intrinsic (sanitizer output checked)');print(report[-1],flush=True)
    (ROOT/'METHOD-RESULTS.txt').write_text('\n'.join(report)+'\nHost only; calculator execution not verified.\n',encoding='utf-8')
