"""Compare actual OpenJDK regex execution, character APIs and numeric parsing."""
import argparse,difflib,os,zipfile
from pathlib import Path
from test import ROOT,tool,path_for,run
ap=argparse.ArgumentParser();ap.add_argument('--vm',default='build/nspire-jvm');ap.add_argument('--only');ap.add_argument('--xinbot',type=Path);ns=ap.parse_args()
java,javac=tool('java'),tool('javac');build=ROOT/'build/regex-tests';build.mkdir(parents=True,exist_ok=True)
run([javac,'--release','8','-encoding','UTF-8','-d',path_for(javac,build),*[path_for(javac,p) for p in sorted((ROOT/'regex-tests').glob('*.java'))]])
jar=build/'regex.jar'
with zipfile.ZipFile(jar,'w',zipfile.ZIP_DEFLATED) as z:
    for p in build.glob('*.class'):z.write(p,p.name)
report=[]
def check(name,paths):
    sep=';' if java.lower().endswith('.exe') else os.pathsep
    expected=run([java,'-Dfile.encoding=UTF-8','-cp',sep.join(path_for(java,p) for p in paths),name]).stdout
    result=run([str(Path(ns.vm).resolve()),'--heap','524288','--steps','1000000000','-bootclasspath',ROOT/'dist/runtime.jar.tns','-cp',';'.join(str(p) for p in paths),name],timeout=240)
    assert result.stdout==expected,''.join(difflib.unified_diff(expected.splitlines(True),result.stdout.splitlines(True),fromfile='Java',tofile='Nspire JVM'))
    report.append('PASS '+name+' (standard Java oracle)');print(report[-1],flush=True)
    return result.stdout
for name in ('RegexTest','CharacterTest','ParseNumberTest','CharacterPropertiesTest'):
    if not ns.only or ns.only==name:check(name,[jar])
if ns.xinbot and (not ns.only or ns.only=='LogbackDurationTest'):
    xinbot=ns.xinbot.resolve()
    run([javac,'--release','8','-cp',path_for(javac,xinbot),'-d',path_for(javac,build),path_for(javac,ROOT/'regex-tests/logback/LogbackDurationTest.java')])
    archive=build/'logback-duration.jar'
    with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED) as z:z.write(build/'LogbackDurationTest.class','LogbackDurationTest.class')
    result=check('LogbackDurationTest',[archive,xinbot])
    (ROOT/'LOGBACK-DURATION-RESULTS.txt').write_text(report[-1]+'\n'+result+'\nComponent check only; full Xinbot startup is not verified.\n')
if not ns.only or ns.only=='EnvironmentTest':
    variables={'NSPIRE_JVM_TEST_7311':'env-中-😀','NSPIRE_JVM_TEST_中_7311':'value','NSPIRE_JVM_TEST_EMPTY_7311':'','NSPIRE_JVM_TEST_MISSING_7311':None}
    old={name:os.environ.get(name) for name in variables}
    try:
        for name,value in variables.items():
            if value is None:os.environ.pop(name,None)
            else:os.environ[name]=value
        result=run([str(Path(ns.vm).resolve()),'-bootclasspath',ROOT/'dist/runtime.jar.tns','-cp',jar,'EnvironmentTest'])
        assert result.stdout=='controlled process environment lookup passed\n',result.stdout
    finally:
        for name,value in old.items():
            if value is None:os.environ.pop(name,None)
            else:os.environ[name]=value
    report.append('PASS controlled environment lookup (synthetic test variables only)');print(report[-1],flush=True)
if not ns.only:
    for mode,missing in [('normalize','java/text/Normalizer$Form'),('script','java/lang/Character$UnicodeScript')]:
        result=run([str(Path(ns.vm).resolve()),'-bootclasspath',ROOT/'dist/runtime.jar.tns','-cp',jar,'UnsupportedRegexTest',mode],ok=False)
        assert result.returncode==1 and 'class not found: '+missing in result.stderr,(result.stdout,result.stderr)
        report.append('PASS explicit unsupported regex '+mode+' (sanitizer output checked)');print(report[-1],flush=True)
    (ROOT/'REGEX-RESULTS.txt').write_text('\n'.join(report)+'\nHost only; calculator execution not verified.\n')
