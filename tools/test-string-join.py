"""Java 8 String.join behavior, including CharSequence callbacks during GC."""
import argparse
from pathlib import Path
from test import ROOT, tool, path_for, run
ap=argparse.ArgumentParser();ap.add_argument('--java',required=True);ap.add_argument('--vm',default='build/nspire-jvm');ns=ap.parse_args()
build=ROOT/'build/join-tests';build.mkdir(parents=True,exist_ok=True);javac=tool('javac')
run([javac,'--release','8','-encoding','UTF-8','-d',path_for(javac,build),path_for(javac,ROOT/'join-tests/StringJoinTest.java')])
expected=run([str(Path(ns.java).resolve()),'-cp',build,'StringJoinTest']).stdout
actual=run([str(Path(ns.vm).resolve()),'--heap','131072','-bootclasspath',ROOT/'dist/runtime.jar.tns','-cp',build,'StringJoinTest']).stdout
assert actual==expected,(expected,actual)
report='PASS both String.join overloads: Java 8 output, nulls, UTF-16, custom sequences/iterators, exceptions and GC\n'
print(report,end='');(ROOT/'STRING-JOIN-RESULTS.txt').write_text(report)
