"""Reproduce OpenJDK 8 heap buffers, views, charset coders and exceptions.

Uses preserved upstream make rules/scripts and Spp. No JRE classfiles are copied.
Run in Linux/WSL; --java is a Linux JRE, javac may be the Windows JDK.
"""
import argparse,hashlib,json,os,shlex,subprocess,tempfile
from pathlib import Path
from test import ROOT,tool,path_for

ap=argparse.ArgumentParser();ap.add_argument('--java',required=True);ap.add_argument('--update',action='store_true');ns=ap.parse_args()
vendor=ROOT/'vendor/openjdk8-nio';manifest=json.loads((vendor/'SOURCES.json').read_text())
for name,record in manifest['files'].items():
 assert hashlib.sha256((vendor/name).read_bytes()).hexdigest()==record['sha256'],name
jdk=vendor/'jdk';javac=tool('javac');build=ROOT/'build';build.mkdir(exist_ok=True)
with tempfile.TemporaryDirectory(prefix='nio-',dir=build) as temp:
 out=Path(temp);classes=out/'spp';classes.mkdir()
 subprocess.run([javac,'--release','8','-d',path_for(javac,classes),path_for(javac,jdk/'make/src/classes/build/tools/spp/Spp.java')],check=True)
 wrapper=out/'generate.mk'
 wrapper.write_text('''.DEFAULT_GOAL := all
define NEWLINE


endef
ECHO := echo
MKDIR := mkdir
TOUCH := touch
MV := mv
RM := rm -f
SED := sed
PRINTF := printf
NAWK := awk
SH := /bin/sh
JDK_TOPDIR := '''+str(jdk)+'''
JDK_OUTPUTDIR := '''+str(out)+'''
TOOL_SPP := '''+shlex.join([str(Path(ns.java).resolve()),'-cp',str(classes),'build.tools.spp.Spp'])+'''
include $(JDK_TOPDIR)/make/gensrc/GensrcBuffer.gmk
include $(JDK_TOPDIR)/make/gensrc/GensrcCharsetCoder.gmk
include $(JDK_TOPDIR)/make/gensrc/GensrcExceptions.gmk
all: $(filter-out $(GENSRC_BUFFER_DST)/Direct%,$(GENSRC_BUFFER)) $(GENSRC_CHARSETCODER) $(GENSRC_EXCEPTIONS_DST)/_the.. $(GENSRC_EXCEPTIONS_DST)/_the.charset
''')
 subprocess.run(['make','--silent','-f',str(wrapper)],check=True)
 # The provider is project code; aliases below are preserved upstream data.
 groups={};current=None
 for line in (jdk/'src/share/classes/sun/nio/cs/standard-charsets').read_text().splitlines():
  words=line.split('#',1)[0].split()
  if not words:continue
  if words[0]=='charset':current=words[2];groups[current]=[]
  elif words[0]=='alias':groups[current].append(words[1])
 aliases=out/'gensrc/sun/nio/cs/NspireAliases.java';aliases.parent.mkdir(parents=True)
 header=(jdk/'src/share/classes/java/nio/Buffer.java').read_text() if (jdk/'src/share/classes/java/nio/Buffer.java').exists() else (ROOT/'runtime/openjdk8/java/nio/Buffer.java').read_text()
 aliases.write_text(header.split('package java.nio;',1)[0]+'package sun.nio.cs;\n// Generated from the preserved standard-charsets alias data.\nfinal class NspireAliases {\n'+''.join('    static final String[] '+name+' = {'+','.join(json.dumps(s) for s in groups[name])+'};\n' for name in ('US_ASCII','ISO_8859_1','UTF_8','UTF_16','UTF_16BE','UTF_16LE'))+'}\n')
 generated={}
 for p in sorted((out/'gensrc').rglob('*.java')):
  name='generated/'+p.relative_to(out/'gensrc').as_posix();data=p.read_bytes()
  digest=hashlib.sha256(data).hexdigest()
  if not ns.update:assert manifest['generated'][name]['sha256']==digest,name
  target=vendor/name;target.parent.mkdir(parents=True,exist_ok=True)
  if not target.exists() or target.read_bytes()!=data:target.write_bytes(data)
  generated[name]={'sha256':digest}
 if ns.update:
  manifest['generated']=generated;(vendor/'SOURCES.json').write_text(json.dumps(manifest,indent=2)+'\n')
 else:assert set(generated)==set(manifest['generated'])
 print('Generated/verified',len(generated),'OpenJDK NIO sources')
