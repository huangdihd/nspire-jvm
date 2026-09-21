"""Differential classpath, service-discovery and input-stream tests."""
import argparse,os,zipfile
from pathlib import Path
from test import ROOT,tool,path_for,run

def main():
    ap=argparse.ArgumentParser();ap.add_argument('--vm',default='build/nspire-jvm');ns=ap.parse_args()
    javac,java=tool('javac'),tool('java');vm=str(Path(ns.vm).resolve())
    build=ROOT/'build/loader-tests';classes=build/'classes';classes.mkdir(parents=True,exist_ok=True)
    sources=sorted((ROOT/'loader-tests').rglob('*.java'))
    run([javac,'--release','8','-encoding','UTF-8','-d',path_for(javac,classes),*[path_for(javac,p) for p in sources]])
    service='META-INF/services/svc.ServiceTest$Greeting'
    config1='# UTF-8 注释\n svc.ServiceTest$ProviderA # first\nsvc.ServiceTest$ProviderA\n'
    config2='svc.ServiceTest$ProviderB\r\nsvc.ServiceTest$中文Provider\nsvc.ServiceTest$ProviderA\n'
    app,other=build/'app with spaces.jar',build/'other.jar'
    directory=build/'other directory';(directory/Path(service).parent).mkdir(parents=True,exist_ok=True)
    (directory/'svc').mkdir(exist_ok=True)
    (directory/service).write_text(config2,encoding='utf-8');(directory/'svc/shared.txt').write_text('second\n')
    with zipfile.ZipFile(app,'w',zipfile.ZIP_DEFLATED) as a,zipfile.ZipFile(other,'w',zipfile.ZIP_DEFLATED) as b:
        for p in classes.rglob('*.class'):
            rel=p.relative_to(classes)
            if rel.name in ('ServiceTest$ProviderB.class','ServiceTest$中文Provider.class'):
                b.write(p,rel.as_posix());target=directory/rel;target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(p.read_bytes())
            else:a.write(p,rel.as_posix())
        a.writestr(service,config1);a.writestr('svc/shared.txt','first\n')
        b.writestr(service,config2);b.writestr('svc/shared.txt','second\n')
    report=[]
    def check(name,cp,label):
        sep=';' if java.lower().endswith('.exe') else os.pathsep
        expected=run([java,'-Dfile.encoding=UTF-8','-cp',sep.join(path_for(java,p) for p in cp),name]).stdout
        result=run([vm,'-bootclasspath',ROOT/'dist/runtime.jar.tns','-cp',';'.join(map(str,cp)),name])
        assert result.stdout==expected,(label,expected,result.stdout,result.stderr)
        report.append('PASS '+label);print(report[-1],flush=True)
    check('StreamsTest',[app],'UTF-8 streams, byte windows, close and bounds')
    check('ReflectionTest',[app],'constructors, access, unboxing, GC roots and target exceptions')
    check('ConnectionTest',[app,directory],'JAR/file connections, settings, stream lifetime and metadata')
    check('svc.ServiceTest',[app,other],'services across two JARs, duplicates, lazy init and reload')
    check('svc.ServiceTest',[app,directory],'services across JAR and directory')
    for label,config in [('syntax','svc.Bad Provider'),('missing','svc.Missing'),('type','svc.ServiceTest$NotProvider'),('constructor','svc.ServiceTest$Broken')]:
        bad=build/(label+'.jar')
        with zipfile.ZipFile(bad,'w',zipfile.ZIP_DEFLATED) as z:z.writestr(service,config+'\n')
        check('svc.BadServiceTest',[bad,app],'service error: '+label)
    (ROOT/'LOADER-RESULTS.txt').write_text('\n'.join(report)+'\nHost only; calculator execution not verified.\n')

if __name__=='__main__':main()
