"""Extract Java 8 method declarations for the VM's intrinsic classes.
Only names, descriptors, access flags and declared exception names are emitted;
no implementation bytecode is copied. Input rt.jar is a build-time dependency.
"""
import argparse,hashlib,json,re,struct,zipfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
ap=argparse.ArgumentParser();ap.add_argument('--rt-jar',type=Path,required=True);ap.add_argument('--update',action='store_true',help='Update the output hash after a reviewed generator/registry change');ns=ap.parse_args()
target=ROOT/'vendor/openjdk8-api';target.mkdir(parents=True,exist_ok=True)
manifest=target/'SOURCES.json';digest=hashlib.sha256(ns.rt_jar.read_bytes()).hexdigest()
if manifest.exists() and json.loads(manifest.read_text())['rt_jar_sha256']!=digest:raise RuntimeError('rt.jar differs from pinned input')
source=(ROOT/'src/vm.c').read_text()
section=source[source.index('static const char *wrapper_primitive'):source.index('static int valid_name')]
names=sorted({n for n in re.findall(r'"((?:java|sun)/[^";\[ ]+)"',section) if not n.endswith('/')})
names=sorted(set(names)|{'java/lang/'+n for n in ('Boolean','Byte','Character','Short','Integer','Long','Float','Double','Void')})
rows=[]
class Reader:
    def __init__(self,data):self.data=data;self.pos=0
    def take(self,n):r=self.data[self.pos:self.pos+n];assert len(r)==n;self.pos+=n;return r
    def u(self,n):return int.from_bytes(self.take(n),'big')
def skip_attributes(r):
    for _ in range(r.u(2)):r.u(2);r.take(r.u(4))
with zipfile.ZipFile(ns.rt_jar) as z:
    for name in names:
        r=Reader(z.read(name+'.class'));assert r.u(4)==0xcafebabe;r.u(2);assert r.u(2)==52
        cp=[None]*r.u(2);i=1
        while i<len(cp):
            tag=r.u(1)
            if tag==1:cp[i]=r.take(r.u(2)).decode('utf-8')
            elif tag==7:cp[i]=r.u(2)
            elif tag in (3,4,9,10,11,12,18):r.take(4)
            elif tag in (5,6):r.take(8);i+=1
            elif tag in (8,16):r.take(2)
            elif tag==15:r.take(3)
            else:raise RuntimeError(tag)
            i+=1
        r.take(6);r.take(r.u(2)*2)
        for _ in range(r.u(2)):r.take(6);skip_attributes(r)
        for _ in range(r.u(2)):
            flags,n,d=r.u(2),cp[r.u(2)],cp[r.u(2)];exceptions=[]
            for _ in range(r.u(2)):
                attr=cp[r.u(2)];data=r.take(r.u(4))
                if attr=='Exceptions':
                    a=Reader(data);exceptions=[cp[cp[a.u(2)]] for _ in range(a.u(2))]
                if attr=='MethodParameters' and n not in ('<init>','<clinit>'):
                    raise RuntimeError('Intrinsic parameter names must be imported before changing the pinned API: '+name+'.'+n)
            if n not in ('<init>','<clinit>'):rows.append((name,n,d,flags,';'.join(exceptions)))
out='/* Generated Java 8 API declarations; see NOTICE and SOURCES.json. */\n'
out+='static const struct BuiltinMethodRecord { const char *owner,*name,*desc;unsigned flags;const char *exceptions; } builtin_method_records[] = {\n'
for owner,name,desc,flags,exc in rows:out+='    {'+','.join([json.dumps(owner),json.dumps(name),json.dumps(desc),hex(flags),json.dumps(exc)])+'},\n'
out+='};\n';data=out.encode();dest=target/'methods.inc'
if manifest.exists() and not ns.update and json.loads(manifest.read_text())['generated']['methods.inc']['sha256']!=hashlib.sha256(data).hexdigest():raise RuntimeError('Generated API differs from manifest; review changes explicitly')
if not dest.exists() or dest.read_bytes()!=data:dest.write_bytes(data)
if not manifest.exists() or ns.update:
    old=json.loads(manifest.read_text()) if manifest.exists() else {}
    old.update({'source':'Temurin 8u504-b01 JRE build-time rt.jar (API declarations only)','rt_jar_sha256':digest,'classes':len(names),'methods':len(rows),'method_parameters_attributes':0,'files':old.get('files',{}),'generated':{'methods.inc':{'sha256':hashlib.sha256(data).hexdigest()}}})
    manifest.write_text(json.dumps(old,indent=2)+'\n')
print('Generated',len(rows),'method declarations for',len(names),'intrinsic classes')
