"""Compare runtime-visible annotation behavior against a standard Java VM."""
import argparse,difflib,zipfile,tempfile,os
from pathlib import Path
from test import ROOT,tool,path_for,run
ap=argparse.ArgumentParser();ap.add_argument('--vm',default='build/nspire-jvm');ap.add_argument('--only');ap.add_argument('--xinbot',type=Path);ns=ap.parse_args()
java,javac=tool('java'),tool('javac');build=ROOT/'build/annotation-tests';build.mkdir(parents=True,exist_ok=True)
sources=sorted((ROOT/'annotation-tests').glob('*.java'))
run([javac,'--release','8','-encoding','UTF-8','-d',path_for(javac,build),*[path_for(javac,p) for p in sources]])
jar=build/'annotations.jar'
with zipfile.ZipFile(jar,'w',zipfile.ZIP_DEFLATED) as z:
    for p in build.glob('*.class'):z.write(p,p.name)
report=[]
for name in ('AnnotationTest','RepeatableTest','PrimitiveAnnotationTest'):
    if ns.only and ns.only!=name:continue
    expected=run([java,'-cp',path_for(java,jar),name]).stdout
    result=run([str(Path(ns.vm).resolve()),'--heap','262144','-bootclasspath',ROOT/'dist/runtime.jar.tns','-cp',jar,name])
    assert result.stdout==expected,''.join(difflib.unified_diff(expected.splitlines(True),result.stdout.splitlines(True),fromfile='Java',tofile='Nspire JVM'))
    report.append('PASS '+name+' (standard Java oracle)');print(report[-1],flush=True)
if not ns.only or ns.only=='EvolutionTest':
    with tempfile.TemporaryDirectory(prefix='evolution-',dir=build) as temp:
        directory=Path(temp)
        def compile_files(files):
            run([javac,'--release','8','-d',path_for(javac,directory),'-cp',path_for(javac,directory),*[path_for(javac,p) for p in files]])
        source=ROOT/'annotation-tests/evolution'
        compile_files([source/'v1/Evolved.java'])
        compile_files(sorted((source/'v2').glob('*.java')))
        archive=directory/'evolved.jar'
        with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED) as z:
            for p in directory.glob('*.class'):
                if p.name not in ('Removed.class','Vanished.class'):z.write(p,p.name)
        expected=run([java,'-cp',path_for(java,archive),'EvolutionTest']).stdout
        result=run([str(Path(ns.vm).resolve()),'--heap','131072','-bootclasspath',ROOT/'dist/runtime.jar.tns','-cp',archive,'EvolutionTest'])
        assert result.stdout==expected,(expected,result.stdout)
        report.append('PASS EvolutionTest (standard Java oracle, replaced/removed classfiles)');print(report[-1],flush=True)
if ns.xinbot and (not ns.only or ns.only=='LogbackAnnotationTest'):
    xinbot=ns.xinbot.resolve()
    with zipfile.ZipFile(xinbot) as z:
        names=sorted(n[:-6].replace('/','.') for n in z.namelist() if n.endswith('.class') and b'Lch/qos/logback/core/model/processor/PhaseIndicator;' in z.read(n))
    run([javac,'--release','8','-cp',path_for(javac,xinbot),'-d',path_for(javac,build),path_for(javac,ROOT/'annotation-tests/logback/LogbackAnnotationTest.java')])
    archive=build/'logback-annotations.jar'
    with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED) as z:z.write(build/'LogbackAnnotationTest.class','LogbackAnnotationTest.class')
    separator=';' if java.lower().endswith('.exe') else os.pathsep
    expected=run([java,'-cp',separator.join(path_for(java,p) for p in (archive,xinbot)),'LogbackAnnotationTest',*names]).stdout
    result=run([str(Path(ns.vm).resolve()),'-bootclasspath',ROOT/'dist/runtime.jar.tns','-cp',str(archive)+';'+str(xinbot),'LogbackAnnotationTest',*names])
    assert result.stdout==expected,(expected,result.stdout)
    report.append('PASS actual Logback annotation phases ('+str(len(names))+' upstream classes, standard Java oracle)');print(report[-1],flush=True)
    (ROOT/'LOGBACK-ANNOTATION-RESULTS.txt').write_text(report[-1]+'\n'+result.stdout+'\nThis component check does not prove full Xinbot startup.\n')
if not ns.only:
    result=run([str(Path(ns.vm).resolve()),'-bootclasspath',ROOT/'dist/runtime.jar.tns','-cp',jar,'UnsupportedAnnotationTextTest'],ok=False)
    assert result.returncode==1 and 'annotation floating-point text formatting is not implemented' in result.stderr,(result.stdout,result.stderr)
    report.append('PASS explicit failure for annotation floating-point text (sanitizer output checked)');print(report[-1],flush=True)
    (ROOT/'ANNOTATION-RESULTS.txt').write_text('\n'.join(report)+'\nHost only; calculator execution not verified.\n')
