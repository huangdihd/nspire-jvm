"""Compare output streams with Java, including unchanged Logback wrappers.
Use Java 8 for the imported FilterOutputStream's historical close semantics.
"""
import argparse,os,re,subprocess
from pathlib import Path
from test import ROOT,tool,path_for,run
ap=argparse.ArgumentParser();ap.add_argument('--vm',default='build/nspire-jvm');ap.add_argument('--java',required=True,help='Java 8 runtime for matching upstream stream semantics');ap.add_argument('--xinbot',type=Path);ap.add_argument('--only');ns=ap.parse_args()
java=str(Path(ns.java).resolve());javac=tool('javac');vm=str(Path(ns.vm).resolve());build=ROOT/'build/output-tests';build.mkdir(parents=True,exist_ok=True)
version=run([java,'-version']).stderr
if 'version "1.8.' not in version:raise RuntimeError('OutputStream oracle must be Java 8: '+version)
run([javac,'--release','8','-encoding','UTF-8','-d',path_for(javac,build),*[path_for(javac,p) for p in sorted((ROOT/'output-tests').glob('*.java'))]])
report=[]
def binary(command):
    result=subprocess.run(list(map(str,command)),capture_output=True,timeout=60)
    errors=result.stderr.decode('utf-8','replace')
    if result.returncode or any(marker in errors for marker in ('ERROR: AddressSanitizer','ERROR: LeakSanitizer','SUMMARY: UndefinedBehaviorSanitizer','runtime error:')):
        raise AssertionError(str(command)+'\n'+repr(result.stdout)+'\n'+errors)
    return result
def check(name,paths,oracle='Java 8'):
    sep=';' if java.lower().endswith('.exe') else os.pathsep
    reference=[java,'-Dfile.encoding=UTF-8','-Dline.separator=\n','-cp',sep.join(path_for(java,p) for p in paths),name]
    command=[vm,'--heap','524288' if name=='OutputConcurrencyTest' else '131072','-bootclasspath',ROOT/'dist/runtime.jar.tns','-cp',';'.join(str(p) for p in paths),name]
    expected=binary(reference);actual=binary(command)
    for channel in ('stdout','stderr'):
        left,right=getattr(expected,channel),getattr(actual,channel)
        if name=='OutputConcurrencyTest' and channel=='stderr':
            # This exact ASan runtime warning concerns ucontext instrumentation,
            # not Java stderr. Keep all other diagnostics and check errors first.
            right=re.sub(rb"(?m)^==[0-9]+==WARNING: ASan doesn't fully support makecontext/swapcontext functions and may produce false positives in some cases!\r?\n",b'',right,count=1)
        if left!=right:raise AssertionError(name+' '+channel+' differs\nExpected: '+repr(left)+'\nActual: '+repr(right))
    report.append('PASS '+name+' ('+oracle+' exact stdout and stderr bytes)');print(report[-1],flush=True)
    return actual.stdout.decode('utf-8','replace')
for name in ('OutputStreamTest','PrintStreamTest','OutputFailureTest','ConsoleOutputTest','OutputConcurrencyTest'):
    if not ns.only or ns.only==name:check(name,[build])
if ns.xinbot and (not ns.only or ns.only=='LogbackConsoleTest'):
    xinbot=ns.xinbot.resolve()
    run([javac,'--release','8','-cp',path_for(javac,xinbot),'-d',path_for(javac,build),path_for(javac,ROOT/'output-tests/logback/LogbackConsoleTest.java')])
    # Original Logback bytecode requires Java 17; the wrapper operations tested
    # here do not depend on Java 8's FilterOutputStream.close implementation.
    previous=java;java=tool('java');result=check('LogbackConsoleTest',[build,xinbot],'Java 17');java=previous
    (ROOT/'LOGBACK-CONSOLE-RESULTS.txt').write_text(report[-1]+'\n'+result+'\nComponent test only; full Xinbot startup is not verified.\n',encoding='utf-8')
if not ns.only:
    filename=build/'unsupported-printstream-file.txt'
    if filename.exists():raise RuntimeError('Unexpected test output already exists: '+str(filename))
    result=run([vm,'-bootclasspath',ROOT/'dist/runtime.jar.tns','-cp',build,'UnsupportedOutputTest',filename],ok=False)
    assert result.returncode==1 and 'PrintStream method is not implemented: <init>(Ljava/lang/String;)V' in result.stderr,result.stderr
    assert not filename.exists(),'Unsupported constructor created a file'
    report.append('PASS explicit unsupported file constructor (no file created; sanitizer output checked)');print(report[-1],flush=True)
    (ROOT/'OUTPUT-RESULTS.txt').write_text('\n'.join(report)+'\nHost only; calculator execution not verified.\n',encoding='utf-8')
