"""Real javac 17 records, compared with Linux Java 17; optional original Xinbot."""
import argparse, difflib, os, shutil
from pathlib import Path
from test import ROOT, tool, path_for, run

ap=argparse.ArgumentParser();ap.add_argument('--java',required=True);ap.add_argument('--vm',default='build/nspire-jvm');ap.add_argument('--xinbot',type=Path);ns=ap.parse_args()
java=str(Path(ns.java).resolve());vm=str(Path(ns.vm).resolve());javac=tool('javac')
build=ROOT/'build/record-tests';build.mkdir(parents=True,exist_ok=True)
sources=[p for p in sorted((ROOT/'record-tests').glob('*.java')) if p.name!='XinbotRecordTest.java']
run([javac,'--release','17','-encoding','UTF-8','-d',path_for(javac,build),*[path_for(javac,p) for p in sources]])
report=[]
def compare(name,classpath,heap='262144'):
    reference=run([java,'-Dfile.encoding=UTF-8','-cp',os.pathsep.join(map(str,classpath)),name],timeout=90).stdout
    result=run([vm,'--heap',heap,'-bootclasspath',ROOT/'dist/runtime.jar.tns','-cp',';'.join(map(str,classpath)),name],timeout=90).stdout
    assert result==reference,name+'\n'+''.join(difflib.unified_diff(reference.splitlines(True),result.splitlines(True)))
    message='PASS '+name+' (Java 17 output; real record fields, callbacks and GC)'
    print(message,flush=True);report.append(message)
compare('RecordTest',[build])
compare('RecordFloatingTest',[build],'2097152')
compare('IntegralParsingTest',[build],'8388608')
# Change the actual compiler output, rather than bypassing the call site.
original=(build/'RecordTest$Pair.class').read_bytes()
position=8
def read(width):
    global position
    value=int.from_bytes(original[position:position+width],'big');position+=width;return value
pool=[None]*read(2);index=1
while index<len(pool):
    start=position;tag=read(1);record={'tag':tag,'start':start}
    if tag==1:
        length=read(2);record['data']=position;record['text']=original[position:position+length].decode('utf-8');position+=length
    elif tag in (3,4):position+=4
    elif tag in (5,6):position+=8
    elif tag in (7,8,16,19,20):record['a']=read(2)
    elif tag in (9,10,11,12,17,18):record['a']=read(2);record['b']=read(2)
    elif tag==15:record['kind']=read(1);record['a']=read(2)
    else:raise AssertionError(tag)
    pool[index]=record;index+=2 if tag in (5,6) else 1
for case,diagnostic,java_error in [
    ('names','component name count mismatch','BootstrapMethodError'),
    ('handle','accessor handle kind is not implemented','IllegalAccessError'),
    ('operation','invalid ObjectMethods operation','BootstrapMethodError'),
    ('attribute',None,None)]:
    folder=build/case;folder.mkdir(exist_ok=True)
    for path in build.glob('*.class'):shutil.copyfile(path,folder/path.name)
    data=bytearray(original)
    if case=='names':
        item=next(p for p in pool if p and p.get('text')=='number;label');data[item['data']+6]=ord('_')
    elif case=='handle':
        item=next(p for p in pool if p and p['tag']==15 and p['kind']==1);data[item['start']+1]=2
    elif case=='operation':
        item=next(p for p in pool if p and p['tag']==18 and pool[pool[p['b']]['a']].get('text')=='toString')
        nt=pool[item['b']];name=next(i for i,p in enumerate(pool) if p and p.get('text')=='number')
        data[nt['start']+1:nt['start']+3]=name.to_bytes(2,'big')
    else:
        item=next(p for p in pool if p and p.get('text')=='Record');data[item['data']]=ord('X')
    (folder/'RecordTest$Pair.class').write_bytes(data)
    if diagnostic:
        reference=run([java,'-cp',folder,'RecordInvalidTest'],ok=False)
        actual=run([vm,'-bootclasspath',ROOT/'dist/runtime.jar.tns','-cp',folder,'RecordInvalidTest'],ok=False)
        assert reference.returncode!=0 and java_error in reference.stderr,reference.stderr
        assert actual.returncode==1 and diagnostic in actual.stderr,actual.stderr
        line='PASS malformed ObjectMethods '+case+' (Java rejects; VM explicitly diagnoses unsupported/invalid metadata)'
    else:
        reference=run([java,'-cp',folder,'RecordFlagTest']).stdout
        actual=run([vm,'-bootclasspath',ROOT/'dist/runtime.jar.tns','-cp',folder,'RecordFlagTest']).stdout
        assert actual==reference=='false\n',(reference,actual)
        line='PASS isRecord requires the classfile Record attribute'
    report.append(line);print(line,flush=True)
if ns.xinbot:
    jar=ns.xinbot.resolve()
    run([javac,'--release','17','-cp',path_for(javac,jar),'-d',path_for(javac,build),path_for(javac,ROOT/'record-tests/XinbotRecordTest.java')])
    compare('XinbotRecordTest',[build,jar],'8388608')
(ROOT/'RECORD-RESULTS.txt').write_text('\n'.join(report)+'\nHost checks only; full application startup, RecordComponent reflection, general MethodHandle APIs and calculator execution remain unverified.\n')
