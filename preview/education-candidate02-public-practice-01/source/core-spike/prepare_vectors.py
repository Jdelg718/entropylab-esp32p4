from pathlib import Path
import json
from integrity import verify_fixtures, verify_generated
p=Path(__file__).resolve().parent/'vectors'
manifest = verify_fixtures(p.parent)
rows=[]; seed=''; path=''; pub=''
for line in (p/'bip-0032.mediawiki').read_text().splitlines():
    if '===Test vector 5===' in line: break
    if line.startswith('Seed (hex): '): seed=line.split(': ',1)[1]
    if line.startswith('* Chain '): path=line[len('* Chain '):].replace('<sub>H</sub>',"'")
    if line.startswith('** ext pub: '): pub=line.split(': ',1)[1]
    if line.startswith('** ext prv: '): rows.append((seed,path,pub,line.split(': ',1)[1]))
assert len(rows)==17
bip32='\n'.join('\t'.join(row) for row in rows)+'\n'
v=json.loads((p/'bip39.json').read_text())['english']
bip39='\n'.join('\t'.join(r) for r in v)+'\n'
# Validate both products before any write. Never silently rebaseline hashes.
verify_generated('bip32.tsv', bip32, manifest)
verify_generated('bip39-english.tsv', bip39, manifest)
(p/'bip32.tsv').write_text(bip32)
(p/'bip39-english.tsv').write_text(bip39)
print('BIP32 nodes:',len(rows),'BIP39 English vectors:',len(v))
