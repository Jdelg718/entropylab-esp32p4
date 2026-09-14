import json
import pathlib
import tomllib
p=pathlib.Path(__file__).resolve().parent
raw=json.loads((p/'upstream-expanded.json').read_text())
sub=json.loads((p/'vectors/raw-binary.json').read_text())
fs=[f for f in raw['fixtures'] if f['kind']=='binary' and f['expected'].get('ok') and set(f['input'])<=set('01')]
assert sub['fixtures']==fs and len(fs)==22
expected='\n'.join('\t'.join([f['id'],str(f['words']),f['input'],f['expected']['hex'],f['expected']['mnemonic']]) for f in fs)+'\n'
assert (p/'vectors/raw-binary.tsv').read_text()==expected
for k in ['revision','appSha256','sources']:
    assert sub[k]==raw[k]
a=tomllib.loads((p/'Cargo.lock').read_text())
b=tomllib.loads((p/'../rust/Cargo.lock').read_text())
av={(x['name'],x['version'],x.get('checksum')) for x in a['package']}
assert all((x['name'],x['version'],x.get('checksum')) in av for x in b['package'])
print('PASS 22 exact upstream fixtures, TSV projection and source metadata')
print('PASS all inherited hex-core dependency lock versions and checksums retained')
