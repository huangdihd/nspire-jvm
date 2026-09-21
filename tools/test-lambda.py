"""Compare real javac lambda/method-reference execution with standard Java."""
import argparse, difflib, zipfile
from pathlib import Path
from test import ROOT, tool, path_for, run

def main():
    ap=argparse.ArgumentParser();ap.add_argument('--vm',default='build/nspire-jvm');ns=ap.parse_args()
    vm=str(Path(ns.vm).resolve());javac,java=tool('javac'),tool('java')
    report=[]
    for version in ('8','17'):
        build=ROOT/'build'/('lambda-'+version);build.mkdir(parents=True,exist_ok=True)
        run([javac,'--release',version,'-encoding','UTF-8','-d',path_for(javac,build),*[path_for(javac,p) for p in sorted((ROOT/'lambda-tests').glob('*.java'))]])
        jar=build/'lambda.jar'
        with zipfile.ZipFile(jar,'w',zipfile.ZIP_DEFLATED) as z:
            for p in build.glob('*.class'):z.write(p,p.name)
        expected=run([java,'-cp',path_for(java,jar),'LambdaTest']).stdout
        result=run([vm,'-bootclasspath',ROOT/'dist/runtime.jar.tns','-cp',jar,'--heap','65536','LambdaTest'])
        assert result.stdout==expected,''.join(difflib.unified_diff(expected.splitlines(True),result.stdout.splitlines(True),fromfile='Java',tofile='Nspire JVM'))
        report.append('PASS Java '+version+' captures, method references, adaptation, bridges, defaults, exceptions, threads and GC')
        print(report[-1],flush=True)
    (ROOT/'LAMBDA-RESULTS.txt').write_text('\n'.join(report)+'\nHost only; calculator execution not verified.\n')

if __name__=='__main__':main()
