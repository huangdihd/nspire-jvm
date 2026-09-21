"""Compare OpenJDK library behavior in this VM with a standard Java runtime."""
import argparse
from pathlib import Path
from test import ROOT, path_for, run, tool

def main():
    ap=argparse.ArgumentParser();ap.add_argument('--vm',default='build/nspire-jvm');ns=ap.parse_args()
    javac,java=tool('javac'),tool('java')
    build=ROOT/'build/runtime-tests';build.mkdir(parents=True,exist_ok=True)
    sources=sorted((ROOT/'runtime-tests').glob('*.java'))
    run([javac,'--release','8','-encoding','UTF-8','-d',path_for(javac,build),*[path_for(javac,p) for p in sources]])
    report=[]
    for name in ('CollectionsTest','ConcurrentLibraryTest'):
        expected=run([java,'-cp',path_for(java,build),name]).stdout
        result=run([str(Path(ns.vm).resolve()),'-bootclasspath',ROOT/'dist/runtime.jar.tns','-cp',build,name])
        assert result.stdout==expected,(name,expected,result.stdout,result.stderr)
        report.append('PASS '+name+' (OpenJDK runtime JAR)');print(report[-1],flush=True)
    (ROOT/'RUNTIME-RESULTS.txt').write_text('\n'.join(report)+'\nHost only; calculator execution not verified.\n')

if __name__=='__main__':main()
