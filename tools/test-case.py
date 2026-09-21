"""Compare actual JDK 17 casing, contexts and comparison with this interpreter."""
import argparse, zipfile
from pathlib import Path
from test import ROOT,tool,path_for,run

ap=argparse.ArgumentParser();ap.add_argument('--vm',default='build/nspire-jvm');ns=ap.parse_args()
javac,java=tool('javac'),tool('java');build=ROOT/'build/case-tests';build.mkdir(parents=True,exist_ok=True)
run([javac,'--release','8','-encoding','UTF-8','-d',path_for(javac,build),*[path_for(javac,p) for p in sorted((ROOT/'case-tests').glob('*.java'))]])
jar=build/'case.jar'
with zipfile.ZipFile(jar,'w',zipfile.ZIP_DEFLATED) as z:
    for p in build.glob('*.class'):z.write(p,p.name)
expected=run([java,'-cp',path_for(java,jar),'CaseTest']).stdout
actual=run([str(Path(ns.vm).resolve()),'--heap','131072','--steps','1000000000','-bootclasspath',ROOT/'dist/runtime.jar.tns','-cp',jar,'CaseTest'],timeout=240).stdout
if actual!=expected:
    (build/'expected.txt').write_text(expected);(build/'actual.txt').write_text(actual)
    import difflib
    raise AssertionError(''.join(difflib.unified_diff(expected.splitlines(True),actual.splitlines(True))))
failure=run([str(Path(ns.vm).resolve()),'-bootclasspath',ROOT/'dist/runtime.jar.tns','-cp',jar,'UnsupportedCaseTest'],ok=False)
assert failure.returncode==1 and 'Thai dictionary word boundaries for final sigma are not implemented' in failure.stderr,failure.stderr
report='PASS all Unicode code points: String lower/upper and Character simple mappings/properties\nPASS ROOT/en/zh/tr/az/lt, sigma contexts, expansions, comparisons, nulls, identity and Locale values\nPASS explicit failure for unsupported Thai dictionary boundaries (sanitizer output checked)\nHost differential test against JDK 17; calculator execution not verified.\n'
(ROOT/'CASE-RESULTS.txt').write_text(report);print(report)
