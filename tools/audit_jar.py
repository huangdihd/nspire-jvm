"""Inventory class-file versions and constant-pool features without running a JAR.
This is a structural inventory, not a proof that a JAR will run.
"""
import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import struct
import zipfile

def scan(data):
    offset=0
    def take(n):
        nonlocal offset
        if n > len(data)-offset: raise ValueError('truncated class')
        b=data[offset:offset+n];offset+=n;return b
    def u(n):return int.from_bytes(take(n),'big')
    if u(4)!=0xcafebabe:raise ValueError('bad class magic')
    minor,major=u(2),u(2)
    cp=[None]*u(2); tags=Counter();i=1
    while i<len(cp):
        tag=u(1);tags[tag]+=1
        if tag==1:cp[i]=take(u(2)).decode('utf-8','replace')
        elif tag in (3,4):take(4)
        elif tag in (5,6):take(8);i+=1
        elif tag in (7,8,16,19,20):cp[i]=(tag,u(2))
        elif tag in (9,10,11,12,17,18):cp[i]=(tag,u(2),u(2))
        elif tag==15:take(3)
        else:raise ValueError(f'unknown constant tag {tag}')
        i+=1
    names=[]
    for p in cp:
        if isinstance(p,tuple) and p[0]==7:names.append(cp[p[1]])
    return major,minor,tags,names

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('jar',type=Path);ap.add_argument('--output',type=Path)
    ns=ap.parse_args(); versions=Counter();tags=Counter();refs=Counter();over=[];multi=0;total=0
    with zipfile.ZipFile(ns.jar) as z:
        for entry in z.infolist():
            if not entry.filename.endswith('.class'):continue
            if entry.filename.startswith('META-INF/versions/'):
                multi+=1;continue
            if entry.file_size>2*1024*1024:raise ValueError('class exceeds 2 MiB audit limit')
            major,minor,t,names=scan(z.read(entry));total+=1;versions[major]+=1;tags.update(t)
            refs.update(x for x in names if isinstance(x,str) and x.startswith(('java/','javax/','jdk/','sun/')))
            if major>61 and len(over)<30:over.append({'class':entry.filename,'major':major})
    report={
        'jar':ns.jar.name,'sha256':hashlib.file_digest(ns.jar.open('rb'),'sha256').hexdigest(),
        'base_class_count':total,'excluded_multi_release_classes':multi,
        'class_versions':{str(k):{'java_version':k-44,'classes':v} for k,v in sorted(versions.items())},
        'constant_pool_entries':{'InvokeDynamic':tags[18],'MethodHandle':tags[15],'Dynamic':tags[17]},
        'classes_above_java17_sample':over,
        'top_runtime_class_references':refs.most_common(60),
        'scope':'Structural inventory only, not a compatibility verdict. Constant-pool entries are not execution counts; optional classes may never be loaded.',
    }
    text=json.dumps(report,ensure_ascii=False,indent=2)
    if ns.output:ns.output.write_text(text+'\n',encoding='utf-8')
    else:print(text)

if __name__=='__main__':main()
