"""Real disk side effects and Linux Java 8 comparisons, in disposable fixtures."""
import argparse,difflib,os,resource,subprocess,tempfile
from pathlib import Path
from test import ROOT,tool,path_for,run
ap=argparse.ArgumentParser();ap.add_argument('--vm',default='build/nspire-jvm');ap.add_argument('--java',required=True);ap.add_argument('--lifetime',type=Path);ns=ap.parse_args()
java=str(Path(ns.java).resolve());vm=str(Path(ns.vm).resolve());build=ROOT/'build/file-tests';build.mkdir(parents=True,exist_ok=True)
if java.endswith('.exe'):ap.error('File path comparison requires a Linux Java oracle')
javac=tool('javac');run([javac,'--release','8','-encoding','UTF-8','-d',path_for(javac,build),*[path_for(javac,p) for p in sorted((ROOT/'file-tests').glob('*.java'))]])
report=[]
def fixture(folder):
    folder.mkdir();(folder/'dir').mkdir();(folder/'data').write_bytes(b'input-data');(folder/'alias').symlink_to('dir',target_is_directory=True);(folder/'dangling').symlink_to('missing')
def execute(command,cwd,limit=False,expected=0):
    def limit_fds():resource.setrlimit(resource.RLIMIT_NOFILE,(64,64))
    p=subprocess.run(command,cwd=cwd,capture_output=True,text=True,encoding='utf-8',timeout=60,preexec_fn=limit_fds if limit else None,env=dict(os.environ,LC_ALL='C.UTF-8'))
    assert p.returncode==expected,(command,p.returncode,p.stdout,p.stderr)
    for marker in ('ERROR: AddressSanitizer','ERROR: LeakSanitizer','runtime error:'):assert marker not in p.stderr,p.stderr
    return p
def snapshot(folder):
    return {p.relative_to(folder).as_posix():p.read_bytes() for p in folder.rglob('*') if p.is_file() and not p.is_symlink()}
with tempfile.TemporaryDirectory(prefix='files-中文😀-',dir=build) as temp:
    root=Path(temp)
    for name in ('FilePathTest','FileMutationTest','FileStreamTest','FileDescriptorTest','InputDefaultsTest'):
        oracle=root/(name+'-java');actual=root/(name+'-vm');fixture(oracle);fixture(actual)
        expected=execute([java,'-Dfile.encoding=UTF-8','-Dsun.io.useCanonCaches=false','-cp',str(build),name],oracle).stdout
        got=execute([vm,'--heap','1048576','-bootclasspath',str(ROOT/'dist/runtime.jar.tns'),'-cp',str(build),name],actual).stdout
        assert got==expected,name+'\n'+''.join(difflib.unified_diff(expected.splitlines(True),got.splitlines(True)))
        assert snapshot(actual)==snapshot(oracle),name+' disk contents differ'
        if name=='FileStreamTest':assert (actual/'文字😀.bin').read_bytes()==bytes([0,255,42]) and (actual/'payload').read_bytes()==b''
        if name=='FileDescriptorTest':assert (actual/'shared').read_bytes()==bytes([1,2,3,4,5]) and (actual/'retained').read_bytes()==bytes([17,18])
        report.append('PASS '+name+' (Linux Java 8 output and actual disk contents)');print(report[-1],flush=True)
    for mode in ('input','output','error'):
        captured=[]
        for command in ([java,'-Dfile.encoding=UTF-8','-cp',str(build)],
                        [vm,'--heap','1048576','-bootclasspath',str(ROOT/'dist/runtime.jar.tns'),'-cp',str(build)]):
            # Prefill a pipe before child startup, making available() deterministic.
            readfd,writefd=os.pipe()
            try:
                os.write(writefd,bytes([0,255,1,2,3,4,5,6,7,8,9,10]));os.close(writefd);writefd=-1
                p=subprocess.run([*command,'ConsoleDescriptorTest',mode],stdin=readfd,capture_output=True,timeout=30)
                assert p.returncode==0,(mode,p.returncode,p.stdout,p.stderr)
                captured.append((p.stdout,p.stderr))
            finally:
                os.close(readfd)
                if writefd!=-1:os.close(writefd)
        assert captured[0]==captured[1],(mode,captured)
        report.append('PASS shared console '+mode+' (exact stdout/stderr bytes and pipe input)');print(report[-1],flush=True)
    for fatal in (False,True):
        folder=root/('fatal' if fatal else 'gc');fixture(folder)
        command=[vm,'--heap','524288','-bootclasspath',str(ROOT/'dist/runtime.jar.tns'),'-cp',str(build),'FileGcTest']+(['fatal'] if fatal else [])
        result=execute(command,folder,limit=True,expected=1 if fatal else 0)
        assert result.stdout=='200\n' and (folder/'gc-output').read_bytes()==bytes([7])*200
        assert (folder/'exit-output').read_bytes()==bytes([11])
        if fatal:assert 'String.format conversion is not implemented' in result.stderr,result.stderr
        report.append('PASS file handle GC under 64-descriptor limit'+(' and fatal path' if fatal else ''));print(report[-1],flush=True)
    if ns.lifetime:
        folder=root/'lifetime';fixture(folder)
        result=execute([str(ns.lifetime.resolve()),str(build),str(ROOT/'dist/runtime.jar.tns')],folder,limit=True)
        assert result.stdout.endswith('PASS descriptors unchanged across six normal/fatal VM runs\n'),result.stdout
        assert result.stderr.count('VM error: runtime method not implemented:')==0,result.stderr
        assert (folder/'gc-output').read_bytes()==bytes([7])*1200
        report.append('PASS descriptor count unchanged after six vm_run returns, including three fatal aborts');print(report[-1],flush=True)
(ROOT/'FILE-RESULTS.txt').write_text('\n'.join(report)+'\nHost only; Ndless execution and its unsupported operations remain unverified.\n')
