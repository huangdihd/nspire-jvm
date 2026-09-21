"""Test the real Logback XML reader from a user-supplied Xinbot release JAR."""
import argparse, difflib, hashlib, os
from pathlib import Path
from test import ROOT, tool, path_for, run

def main():
    ap=argparse.ArgumentParser();ap.add_argument('--vm',default='build/nspire-jvm');ap.add_argument('--xinbot',required=True,type=Path);ns=ap.parse_args()
    vm=str(Path(ns.vm).resolve());jar=ns.xinbot.resolve();javac,java=tool('javac'),tool('java')
    build=ROOT/'build/logback-xml';build.mkdir(parents=True,exist_ok=True)
    run([javac,'--release','17','-encoding','UTF-8','-cp',path_for(javac,jar),'-d',path_for(javac,build),path_for(javac,ROOT/'xml-tests/logback/LogbackXmlTest.java')])
    sep=';' if java.lower().endswith('.exe') else os.pathsep
    expected=run([java,'-Dfile.encoding=UTF-8','-cp',sep.join(path_for(java,p) for p in [build,jar]),'LogbackXmlTest']).stdout
    actual=run([vm,'-bootclasspath',ROOT/'dist/runtime.jar.tns','-cp',';'.join(map(str,[build,jar])),'LogbackXmlTest'])
    assert actual.stdout==expected,''.join(difflib.unified_diff(expected.splitlines(True),actual.stdout.splitlines(True),fromfile='Java',tofile='Nspire JVM'))
    report='PASS actual Logback SAX event recording from Xinbot\nXinbot SHA-256: '+hashlib.sha256(jar.read_bytes()).hexdigest()+'\n'+actual.stdout
    (ROOT/'LOGBACK-XML-RESULTS.txt').write_text(report+'Component test only; complete Xinbot startup and calculator execution are not verified.\n',encoding='utf-8')
    print(report,end='')

if __name__=='__main__':main()
