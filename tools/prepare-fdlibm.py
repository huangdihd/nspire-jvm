"""Preserve fdlibm's algorithm while making its negative exponent shift defined."""
from pathlib import Path
import argparse,hashlib,json
ap=argparse.ArgumentParser();ap.add_argument('--update',action='store_true');ns=ap.parse_args()
base=Path(__file__).resolve().parent.parent/'vendor/openjdk8-fdlibm'
manifest=json.loads((base/'SOURCES.json').read_text())
for name,record in manifest['files'].items():
    assert hashlib.sha256((base/name).read_bytes()).hexdigest()==record['sha256'],name
source=(base/'upstream/e_sqrt.c').read_bytes()
before=b'ix0 += (m <<20);'
assert source.count(before)==1
# m is the halved unbiased IEEE-754 exponent, in [-537, 511].
# Multiplication is representable in int32_t, including negative exponents.
data=source.replace(before,b'ix0 += m * 1048576; /* Nspire: defined for negative exponents. */')
name='generated/e_sqrt.c';path=base/name
if ns.update:
    path.parent.mkdir(parents=True,exist_ok=True);path.write_bytes(data)
    manifest['generated']={name:{'sha256':hashlib.sha256(data).hexdigest(),'source':'upstream/e_sqrt.c','change':'Replace signed negative left shift with equivalent representable multiplication.'}}
    (base/'SOURCES.json').write_text(json.dumps(manifest,indent=2)+'\n')
else:
    assert path.read_bytes()==data,'Run prepare-fdlibm.py --update'
    assert hashlib.sha256(data).hexdigest()==manifest['generated'][name]['sha256']
print('fdlibm sources and defined exponent adaptation verified')
