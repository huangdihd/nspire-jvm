"""Compare SAX parsing, Unicode, callbacks and failure cleanup with standard Java."""
import argparse, zipfile, difflib
from pathlib import Path
from test import ROOT, tool, path_for, run

def normalize(text):
    # SAX does not specify ordering among endPrefixMapping callbacks for the
    # same element. Preserve all events and only sort consecutive such calls.
    result=[];pending=[]
    for line in text.splitlines(True):
        if line.startswith('unprefix '):pending.append(line)
        else:
            result.extend(sorted(pending));pending=[];result.append(line)
    return ''.join(result+sorted(pending))

def main():
    ap=argparse.ArgumentParser();ap.add_argument('--vm',default='build/nspire-jvm');ns=ap.parse_args()
    vm=str(Path(ns.vm).resolve());javac,java=tool('javac'),tool('java')
    build=ROOT/'build/xml-tests';build.mkdir(parents=True,exist_ok=True)
    sources=sorted((ROOT/'xml-tests').glob('*.java'))
    run([javac,'--release','8','-encoding','UTF-8','-d',path_for(javac,build),*[path_for(javac,p) for p in sources]])
    fixtures={
        'valid.xml': '<?xml version="1.0"?>\n<?test data?><r xmlns="urn:root" xmlns:p="urn:attr" p:n="中文😃" plain="a &amp; b">\n<p:child>😀x<![CDATA[<text>]]>&lt;</p:child><!--ignored--></r>',
        'plain.xml': '<r z="2" a="1"><c/>x</r>',
        'malformed.xml':'<r>\n<c></r>',
        'duplicate.xml':'<r a="1" a="2"/>',
        'entities.xml':'<!DOCTYPE r [<!ENTITY inner "inside"><!ENTITY ext SYSTEM "file:///not-read-by-disabled-entity"><!ENTITY nested "&inner;&ext;"><!NOTATION image SYSTEM "file:///notation"><!ATTLIST r tokens NMTOKENS "  a   b  " choice (one|two) "one" media NOTATION (image) #IMPLIED>]><r media="image">&nested;tail</r>',
    }
    jar=build/'xml.jar'
    with zipfile.ZipFile(jar,'w',zipfile.ZIP_DEFLATED) as z:
        for p in build.glob('*.class'):z.write(p,p.name)
        for name,xml in fixtures.items():z.writestr(name,xml.encode('utf-8'))
        z.writestr('utf16.xml','<?xml version="1.0" encoding="UTF-16"?><r>中文😀</r>'.encode('utf-16'))
    expected=run([java,'-Dfile.encoding=UTF-8','-cp',path_for(java,jar),'XmlTest']).stdout
    actual=run([vm,'-bootclasspath',ROOT/'dist/runtime.jar.tns','-cp',jar,'XmlTest'])
    assert normalize(actual.stdout)==normalize(expected),''.join(difflib.unified_diff(expected.splitlines(True),actual.stdout.splitlines(True),fromfile='Java',tofile='Nspire JVM'))
    report='PASS SAX events, namespaces, attributes, UTF-8/UTF-16, entities, callbacks, errors and stream cleanup\n'
    expected_nested=run([java,'-cp',path_for(java,jar),'XmlNestedTest']).stdout
    nested=run([vm,'-bootclasspath',ROOT/'dist/runtime.jar.tns','-cp',jar,'--heap','65536','XmlNestedTest'])
    assert nested.stdout==expected_nested,(expected_nested,nested)
    report+='PASS nested parsing, recursive-call rejection, thread callbacks and GC roots\n'
    for args in [[],['child']]:
        aborted=run([vm,'-bootclasspath',ROOT/'dist/runtime.jar.tns','-cp',jar,'XmlAbortTest',*args],ok=False)
        assert aborted.returncode==1 and 'String.format conversion is not implemented' in aborted.stderr,aborted
    report+='PASS VM abort on main/child stacks with active XML parsers (sanitizer output checked)\n'
    (ROOT/'XML-RESULTS.txt').write_text(report+'Host only; calculator execution not verified.\n')
    print(report,end='')

if __name__=='__main__':main()
