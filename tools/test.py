"""Differential tests against a real Java runtime, plus failure-path tests.
Works on Windows, Linux, and WSL using a Windows JDK if no Linux JDK exists.
"""
import argparse
import os
from pathlib import Path
import shutil
import subprocess
import sys
import zipfile

ROOT = Path(__file__).resolve().parents[1]

def tool(name):
    found = shutil.which(name)
    if found:
        return found
    for p in Path('/mnt/c/Program Files/Java').glob(f'*/bin/{name}.exe'):
        return str(p)
    raise RuntimeError(f'{name} not found: install a JDK >= 17')

def path_for(exe, path):
    p = str(Path(path).resolve())
    if exe.lower().endswith('.exe') and sys.platform != 'win32':
        return subprocess.check_output(['wslpath', '-w', p], text=True).strip()
    return p

def run(cmd, ok=True):
    result = subprocess.run(list(map(str,cmd)), text=True, encoding='utf-8', errors='replace', capture_output=True, timeout=45)
    if ok and result.returncode:
        raise AssertionError(f'Failed: {cmd}\n{result.stdout}\n{result.stderr}')
    return result

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--vm', required=True)
    ns = ap.parse_args()
    vm = str(Path(ns.vm).resolve())
    build=ROOT/'build'/'tests'; build.mkdir(parents=True, exist_ok=True)
    javac,java=tool('javac'),tool('java')
    old=[p for p in (ROOT/'tests').glob('*.java') if p.name not in ('ModernTest.java','UnsupportedTest.java')]
    run([javac,'--release','8','-encoding','UTF-8','-d',path_for(javac,build),*[path_for(javac,p) for p in old]])
    for name in ('ModernTest','UnsupportedTest'):
        run([javac,'--release','17','-d',path_for(javac,build),path_for(javac,ROOT/'tests'/f'{name}.java')])
    # Exercise wide local indices and wide iinc using genuine javac bytecode.
    source='public class WideTest { public static void main(String[] args) {\n'
    source+='\n'.join(f'int x{i} = args.length + {i};' for i in range(270))
    source+='\nx269 += 1000; System.out.println('+ '+'.join(f'x{i}' for i in range(270))+'); }}'
    wide=build/'WideTest.java';wide.write_text(source,encoding='utf-8')
    run([javac,'--release','8','-d',path_for(javac,build),path_for(javac,wide)])
    jar=build/'tests.jar'
    with zipfile.ZipFile(jar,'w',zipfile.ZIP_DEFLATED) as z:
        for p in sorted(build.glob('*.class')): z.write(p,p.name)
    cases=[('Demo',[]),('CoreTest',['one','two']),('NumericTest',[]),('GcTest',[]),('ModernTest',[]),('WideTest',[]),('ClassTest',[]),('SyncTest',[]),('ThreadTest',[]),('ThreadGcTest',[]),('ThreadLifecycleTest',[]),('ThreadLocalTest',[]),('PropertiesTest',[])]
    count=0
    report=[]
    for name,args in cases:
        ref=run([java,'-cp',path_for(java,build),name,*args]).stdout
        for cp in (build,jar):
            cmd=[vm,'-cp',cp,'--heap','32768' if name in ('GcTest','ThreadGcTest','ThreadLocalTest') else '8388608',name,*args]
            result=run(cmd)
            if result.stdout!=ref:
                import difflib
                raise AssertionError(name+' output differs\n'+''.join(difflib.unified_diff(ref.splitlines(True),result.stdout.splitlines(True),fromfile='Java',tofile='Nspire JVM')))
            count+=1; report.append(f'PASS {name} ({"JAR" if cp==jar else "directory"})')
            print(report[-1],flush=True)
    negative=[('UnsupportedTest',[],'invokedynamic'),('ThreadFailureTest',[],'invokedynamic'),('LoopTest',['--steps','1000'],'instruction budget')]
    # Split a real application across JARs; runtime resolution must find all
    # dependency classes, not just the entry point. Check boot precedence too.
    appjar,libjar=build/'split-app.jar',build/'split-lib.jar'
    with zipfile.ZipFile(appjar,'w',zipfile.ZIP_DEFLATED) as z:
        z.write(build/'CoreTest.class','CoreTest.class')
    with zipfile.ZipFile(libjar,'w',zipfile.ZIP_DEFLATED) as z:
        for p in build.glob('*.class'):
            if p.name!='CoreTest.class':z.write(p,p.name)
    expected=run([java,'-cp',path_for(java,build),'CoreTest','one','two']).stdout
    for options in (['-cp',str(appjar)+';'+str(libjar)],['-bootclasspath',libjar,'-cp',appjar]):
        assert run([vm,*options,'CoreTest','one','two']).stdout==expected
        count+=1;report.append('PASS split application/runtime classpath');print(report[-1],flush=True)
    # A malformed application shadow must not override a boot class.
    shadow=build/'shadow';shadow.mkdir(exist_ok=True);(shadow/'Derived.class').write_bytes(b'bad class')
    assert run([vm,'-bootclasspath',libjar,'-cp',str(shadow)+';'+str(appjar),'CoreTest','one','two']).stdout==expected
    count+=1;report.append('PASS boot classpath precedence');print(report[-1],flush=True)
    for name,opts,needle in negative:
        result=run([vm,'-cp',jar,*opts,name],ok=False)
        assert result.returncode!=0 and needle in result.stderr,(name,result)
        count+=1;report.append(f'PASS fail-fast {name}');print(report[-1],flush=True)
    bad=build/'broken';bad.mkdir(exist_ok=True)
    demo=(build/'Demo.class').read_bytes()
    for cut in (0,4,12,40,len(demo)//2,len(demo)-1):
        (bad/'Demo.class').write_bytes(demo[:cut])
        result=run([vm,'-cp',bad,'Demo'],ok=False)
        assert result.returncode!=0 and 'VM error:' in result.stderr,result
        count+=1
    report.append('PASS six truncated class files')
    dist=ROOT/'dist';dist.mkdir(exist_ok=True)
    with zipfile.ZipFile(dist/'demo.jar.tns','w',zipfile.ZIP_DEFLATED) as z:
        z.write(build/'Demo.class','Demo.class')
    (dist/'jvm.cfg.tns').write_text('demo.jar.tns\nDemo\n',encoding='ascii')
    report.append(f'{count} checks passed. Host only; no calculator execution verified.')
    (ROOT/'TEST-RESULTS.txt').write_text('\n'.join(report)+'\n',encoding='utf-8')
    print(report[-1])

if __name__=='__main__': main()
