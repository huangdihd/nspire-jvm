"""Package origin and actual JAR manifest values, compared with Linux Java 8."""
import argparse,difflib,os,shutil,zipfile
from pathlib import Path
from test import ROOT,tool,path_for,run
ap=argparse.ArgumentParser();ap.add_argument('--java',required=True);ap.add_argument('--vm',default='build/nspire-jvm');ns=ap.parse_args()
java=str(Path(ns.java).resolve());vm=str(Path(ns.vm).resolve());javac=tool('javac');build=ROOT/'build/package-tests';build.mkdir(parents=True,exist_ok=True)
compiled=build/'compiled';compiled.mkdir(exist_ok=True)
run([javac,'--release','8','-d',path_for(javac,compiled),*[path_for(javac,p) for p in sorted((ROOT/'package-tests').rglob('*.java'))]])
driver=build/'driver';driver.mkdir(exist_ok=True);shutil.copyfile(compiled/'PackageMetadataTest.class',driver/'PackageMetadataTest.class')
loose=build/'loose';(loose/'loose').mkdir(parents=True,exist_ok=True);shutil.copyfile(compiled/'loose/Loose.class',loose/'loose/Loose.class')
(loose/'META-INF').mkdir(exist_ok=True);(loose/'META-INF/MANIFEST.MF').write_bytes(b'Manifest-Version: 1.0\r\nImplementation-Version: ignore-directory-manifest\r\n\r\n')
main=('Manifest-Version: 1.0\r\nSpecification-Title: main-spec\r\nSpecification-Version: 1.2.3\r\nSpecification-Vendor: spec-vendor\r\nImplementation-Title: folded-中-title\r\nImplementation-Version: 2.4.3-RELEASE\r\nImplementation-Vendor: main-vendor\r\n\r\nName: sample/\r\niMpLeMeNtAtIoN-vErSiOn: package-version\r\nImplementation-Vendor: \r\nSpecification-Title: package-spec\r\n\r\n').encode('utf-8')
# Continuation splits a multibyte UTF-8 codepoint; decoding must follow unfolding.
main=main.replace('中'.encode(),b'\xe4\r\n \xb8\xad')
paths=[]
for name,classes,manifest in [('first',['sample/First.class','sample/sub/Third.class'],main),('second',['sample/Second.class'],b'Manifest-Version: 1.0\nImplementation-Version: second-version\n\n'),('absent',['absent/Absent.class'],None)]:
    jar=build/(name+'.jar');paths.append(jar)
    with zipfile.ZipFile(jar,'w',zipfile.ZIP_DEFLATED) as z:
        if manifest:z.writestr('META-INF/MANIFEST.MF',manifest)
        for entry in classes:z.write(compiled/entry,entry)
classpath=[driver,loose,*paths];report=[]
for order in [('sample.First','sample.Second'),('sample.Second','sample.First')]:
    reference=run([java,'-Dfile.encoding=UTF-8','-cp',os.pathsep.join(map(str,classpath)),'PackageMetadataTest',*order]).stdout
    actual=run([vm,'--heap','262144','-bootclasspath',ROOT/'dist/runtime.jar.tns','-cp',';'.join(map(str,classpath)),'PackageMetadataTest',*order]).stdout
    assert reference==actual,''.join(difflib.unified_diff(reference.splitlines(True),actual.splitlines(True)))
    line='PASS first definition '+order[0]+': Java 8 package identity, six manifest fields, sections, folded UTF-8, absence and GC'
    report.append(line);print(line,flush=True)
reference=run([java,'-cp',compiled,'HighByteStringTest']).stdout
actual=run([vm,'-bootclasspath',ROOT/'dist/runtime.jar.tns','-cp',compiled,'HighByteStringTest']).stdout
assert actual==reference,''.join(difflib.unified_diff(reference.splitlines(True),actual.splitlines(True)))
report.append('PASS legacy high-byte String constructors (Java 8 unsigned bytes, UTF-16, slices and exception precedence)');print(report[-1],flush=True)
(ROOT/'PACKAGE-METADATA-RESULTS.txt').write_text('\n'.join(report)+'\nApplication packages on host classpaths; sealing, signing, custom loaders and device execution are not verified.\n')
