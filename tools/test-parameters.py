"""Real MethodParameters and descriptor/annotation metadata against Java 8."""
import argparse,difflib,subprocess,shutil,struct
from pathlib import Path
from test import ROOT,tool,path_for,run
ap=argparse.ArgumentParser();ap.add_argument('--java',required=True);ap.add_argument('--vm',default='build/nspire-jvm');ns=ap.parse_args()
java=str(Path(ns.java).resolve());vm=str(Path(ns.vm).resolve());javac=tool('javac');report=[]
for named in (False,True):
    build=ROOT/'build/parameter-tests'/('named' if named else 'plain');build.mkdir(parents=True,exist_ok=True)
    run([javac,'--release','8','-encoding','UTF-8',*(['-parameters'] if named else []),'-d',path_for(javac,build),*[path_for(javac,p) for p in sorted((ROOT/'parameter-tests').glob('*.java'))]])
    for main in ('ParameterTest','ModifierTest'):
        reference=run([java,'-cp',build,main]).stdout
        actual=run([vm,'--heap','262144','-bootclasspath',ROOT/'dist/runtime.jar.tns','-cp',build,main],timeout=90).stdout
        assert reference==actual,main+'\n'+''.join(difflib.unified_diff(reference.splitlines(True),actual.splitlines(True)))
        line='PASS '+main+(' with MethodParameters' if named else ' without MethodParameters')+' (Java 8 output, GC and defensive copies)'
        report.append(line);print(line,flush=True)
def attributes(data):
    # Locate actual attributes without changing descriptors or application code.
    pos=8
    def take(n):
        nonlocal pos
        result=data[pos:pos+n];pos+=n;return result
    def u(n):return int.from_bytes(take(n),'big')
    cp=[None]*u(2);index=1
    while index<len(cp):
        tag=u(1)
        if tag==1:cp[index]=take(u(2)).decode('utf-8')
        elif tag in (3,4,9,10,11,12,18):take(4)
        elif tag in (5,6):take(8);index+=1
        elif tag in (7,8,16,19,20):take(2)
        elif tag==15:take(3)
        else:raise AssertionError(tag)
        index+=1
    take(6);take(u(2)*2)
    def attrs():
        out=[]
        for _ in range(u(2)):
            name=cp[u(2)];length_pos=pos;size=u(4);start=pos;take(size);out.append((name,length_pos,start,size))
        return out
    for _ in range(u(2)):take(6);attrs()
    found={}
    for _ in range(u(2)):
        u(2);name=cp[u(2)];u(2);entries=attrs()
        if name=='mixed':found={name:(length_pos,start,size) for name,length_pos,start,size in entries}
    return cp,found
named=ROOT/'build/parameter-tests/named';original=(named/'ParameterTest$Fixture.class').read_bytes();cp,found=attributes(original)
for case in ('zero-name','bad-name','bad-index','bad-flags','wrong-count','valid-flags','annotation-count'):
    folder=ROOT/'build/parameter-tests'/case;folder.mkdir(parents=True,exist_ok=True)
    for source in named.glob('*.class'):shutil.copyfile(source,folder/source.name)
    data=bytearray(original);attribute='RuntimeVisibleParameterAnnotations' if case=='annotation-count' else 'MethodParameters'
    length_pos,start,size=found[attribute]
    if case=='zero-name':data[start+1:start+3]=b'\0\0'
    elif case=='bad-name':
        bad=next(i for i,s in enumerate(cp) if s and s.startswith('(') and ';' in s)
        data[start+1:start+3]=struct.pack('>H',bad)
    elif case=='bad-index':data[start+1:start+3]=b'\xff\xff'
    elif case=='bad-flags':data[start+3:start+5]=b'\x40\0'
    elif case=='valid-flags':data[start+3:start+5]=b'\x90\x10'
    else:
        removed=2 if case=='annotation-count' else 4
        data[start]-=1;data[length_pos:length_pos+4]=struct.pack('>I',size-removed)
        del data[start+size-removed:start+size]
    (folder/'ParameterTest$Fixture.class').write_bytes(data)
    arguments=['annotations'] if case=='annotation-count' else []
    reference=run([java,'-cp',folder,'MalformedParameterTest',*arguments]).stdout
    actual=run([vm,'-bootclasspath',ROOT/'dist/runtime.jar.tns','-cp',folder,'MalformedParameterTest',*arguments]).stdout
    assert actual==reference,(case,reference,actual)
    if case in ('bad-name','bad-index','bad-flags','wrong-count'):assert reference=='java.lang.reflect.MalformedParametersException\n',reference
    if case=='annotation-count':assert reference=='java.lang.annotation.AnnotationFormatError\n',reference
    line='PASS classfile '+case+' (Java 8 result/exception)';report.append(line);print(line,flush=True)
p=run([vm,'-bootclasspath',ROOT/'dist/runtime.jar.tns','-cp',named,'GenericParameterUnsupportedTest'],ok=False)
assert p.returncode==1 and p.stdout=='java.util.List\n' and 'generic parameter signatures are not implemented' in p.stderr,(p.stdout,p.stderr)
report.append('PASS generic Signature preserved and unsupported generic reflection fails explicitly');print(report[-1],flush=True)
(ROOT/'PARAMETER-RESULTS.txt').write_text('\n'.join(report)+'\nHost only; full generic/type-annotation reflection and calculator execution are not verified.\n')
