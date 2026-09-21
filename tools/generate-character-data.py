"""Reproduce the checked-in character tables with the pinned Temurin runtime."""
from pathlib import Path
import argparse,hashlib,json,tempfile
from test import ROOT,tool,path_for,run
ap=argparse.ArgumentParser();ap.add_argument('--java',required=True);ns=ap.parse_args()
java=str(Path(ns.java).resolve());javac=tool('javac');build=ROOT/'build/character-generator';build.mkdir(parents=True,exist_ok=True)
run([javac,'--release','17','-d',path_for(javac,build),path_for(javac,ROOT/'tools/GenerateCharacterData.java')])
dest=ROOT/'vendor/openjdk17-casing/character.inc'
manifest=json.loads((dest.parent/'SOURCES.json').read_text())
with tempfile.TemporaryDirectory(prefix='character-',dir=build) as temp:
    output=Path(temp)/'character.inc'
    run([java,'-cp',path_for(java,build),'GenerateCharacterData',path_for(java,output)])
    data=output.read_bytes()
    if hashlib.sha256(data).hexdigest()!=manifest['generated']['character.inc']['sha256']:raise RuntimeError('Generated character data does not match the pinned manifest')
    dest.write_bytes(data)
print('Reproduced character data and verified its pinned SHA-256')
