"""Build the supplemental class library from its preserved upstream sources."""
from pathlib import Path
import argparse, hashlib, json, subprocess, tempfile, zipfile
from test import ROOT, tool, path_for

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--java8-home', required=True, type=Path,
                    help='JDK/JRE 8 directory; rt.jar supplies build-time signatures only')
    ap.add_argument('--javac',help='Compiler executable; Java 8 javac is required for the preserved time sources')
    ns = ap.parse_args()
    candidates = [ns.java8_home / 'jre/lib/rt.jar', ns.java8_home / 'lib/rt.jar']
    rt = next((p for p in candidates if p.is_file()), None)
    if rt is None: ap.error('--java8-home must contain jre/lib/rt.jar or lib/rt.jar')
    javac = ns.javac or tool('javac')
    source = ROOT / 'runtime' / 'openjdk8'
    build = ROOT / 'build'
    build.mkdir(parents=True, exist_ok=True)
    files = sorted(source.rglob('*.java'))
    local = sorted((ROOT / 'runtime' / 'nspire').rglob('*.java'))
    nio = ROOT / 'vendor' / 'openjdk8-nio'
    nio_manifest = json.loads((nio / 'SOURCES.json').read_text())
    generated = sorted((nio / 'generated').rglob('*.java'))
    time_data=ROOT/'vendor/openjdk8-time'
    time_manifest=json.loads((time_data/'SOURCES.json').read_text())
    for path,record in time_manifest['files'].items():
        assert hashlib.sha256((time_data/path).read_bytes()).hexdigest()==record['sha256'],path
    assert {p.relative_to(nio).as_posix() for p in generated} == set(nio_manifest['generated'])
    for path, record in {**nio_manifest['files'], **nio_manifest['generated']}.items():
        assert hashlib.sha256((nio / path).read_bytes()).hexdigest() == record['sha256'], path
    manifest = json.loads((source / 'SOURCES.json').read_text())
    assert {p.relative_to(source).as_posix() for p in files} == set(manifest['files']), 'Source manifest does not match input files'
    for path, record in manifest['files'].items():
        if hashlib.sha256((source / path).read_bytes()).hexdigest() != record['sha256']:
            raise RuntimeError(f'Upstream source hash changed: {path}; document changes before rebuilding')
    # Compile the actual OpenJDK declarations against matching Java 8 APIs,
    # including package-private helpers and Unsafe. Do not redistribute rt.jar.
    archive = ROOT / 'dist' / 'runtime.jar.tns'
    archive.parent.mkdir(parents=True, exist_ok=True)
    # A fresh output directory prevents removed sources leaving stale classes
    # in the distributed JAR. TemporaryDirectory cleans only its own directory.
    with tempfile.TemporaryDirectory(prefix='runtime-', dir=build) as temp:
        output = Path(temp)
        cmd = [javac, '-J-Duser.language=en', '-source', '8', '-target', '8',
               '-bootclasspath', path_for(javac, rt), '-XDignore.symbol.file',
               '-encoding', 'UTF-8', '-classpath', path_for(javac, output), '-d', path_for(javac, output)]
        args = output / 'sources.txt'
        # Compile upstream against the original API. VM adapters intentionally
        # expose subsets and must not alter how upstream bytecode is compiled.
        for sources in (files + generated,local):
            args.write_text('\n'.join('"' + path_for(javac, p).replace('\\', '/') + '"' for p in sources))
            subprocess.run(cmd + ['@' + path_for(javac, args)], check=True)
        with zipfile.ZipFile(archive, 'w', zipfile.ZIP_DEFLATED) as z:
            z.write(time_data/'tzdb.dat','nspire/time/tzdb.dat')
            for name in ('LICENSE','ASSEMBLY_EXCEPTION','THIRD_PARTY_README','SOURCES.json'):
                z.write(time_data/name,'META-INF/openjdk8-time/'+name)
            for p in sorted(output.rglob('*.class')):
                z.write(p, p.relative_to(output).as_posix())
            for name in ('LICENSE', 'ASSEMBLY_EXCEPTION', 'THIRD_PARTY_README', 'SOURCES.json'):
                z.write(source / name, 'META-INF/openjdk8/' + name)
            z.write(ROOT / 'LICENSE', 'META-INF/nspire/LICENSE')
            for name in ('LICENSE', 'ASSEMBLY_EXCEPTION', 'THIRD_PARTY_README', 'SOURCES.json'):
                z.write(nio / name, 'META-INF/openjdk8-nio/' + name)
            z.writestr('META-INF/nspire/SOURCES.json', json.dumps({p.relative_to(ROOT).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest() for p in local}, indent=2))
    print(f'Built {archive.name}: {archive.stat().st_size} bytes')

if __name__ == '__main__': main()
