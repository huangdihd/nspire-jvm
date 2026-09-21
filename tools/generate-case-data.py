"""Regenerate pinned JDK 17 casing data; requires an open-source Java 17 runtime."""
from pathlib import Path
import argparse,hashlib,json,tempfile
from test import ROOT, tool, path_for, run

ap=argparse.ArgumentParser()
ap.add_argument('--java', required=True, help='Path to the pinned Temurin 17 java executable')
ns=ap.parse_args()
javac=tool('javac'); java=str(Path(ns.java).resolve())
build=ROOT/'build/case-generator';build.mkdir(parents=True,exist_ok=True)
run([javac,'--release','17','-d',path_for(javac,build),path_for(javac,ROOT/'tools/GenerateCaseData.java')])
dest=ROOT/'vendor/openjdk17-casing/data.inc';dest.parent.mkdir(parents=True,exist_ok=True)
manifest=json.loads((dest.parent/'SOURCES.json').read_text())
expected=manifest['generated']['data.inc']['sha256']
with tempfile.TemporaryDirectory(prefix='casing-',dir=build) as temp:
    output=Path(temp)/'data.inc'
    run([java,'--add-opens=java.base/java.lang=ALL-UNNAMED','--add-opens=java.base/sun.text=ALL-UNNAMED',
         '-cp',path_for(java,build),'GenerateCaseData',path_for(java,output)])
    data=output.read_bytes()
    if hashlib.sha256(data).hexdigest()!=expected:
        raise RuntimeError('Generated data differs from the pinned source manifest; check runtime version before replacing tables')
    dest.write_bytes(data)
print('Generated',dest)
