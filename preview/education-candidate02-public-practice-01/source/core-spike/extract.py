"""Reproduce small upstream extraction; only visibility/ABI changes, bodies verbatim."""
from pathlib import Path
import subprocess, hashlib, json, os
root = Path(__file__).resolve().parent
up = Path(os.environ.get('ENTROPYLAB_UPSTREAM', root / 'upstream'))
from integrity import verify_fixtures, verify_generated
manifest = verify_fixtures(root)
commit = '6e1f39cc7da25942c7a1f51ea5837f4ac7ef8f51'
if subprocess.check_output(['git','-C',str(up),'rev-parse','HEAD'],text=True).strip() != commit:
    raise SystemExit('upstream revision drift')
blob = subprocess.check_output(['git','-C',str(up),'show',commit+':entropylab-wasm/src/lib.rs'])
if (up/'entropylab-wasm/src/lib.rs').read_bytes() != blob:
    raise SystemExit('upstream working-tree drift')
src = blob.decode('utf-8')
names = ['ctx','wipe','wipe_bytes','wipe_val','wipe_string','wipe_xpriv','read','write78','el_hd_master','el_hd_ckd_priv','el_bip39_entropy_to_mnemonic','network_from_selector','write_script','el_spk_p2wpkh','el_addr_from_script']
imports = '''// Extracted from pinned EntropyLab; run ../extract.py to reproduce.
use bitcoin_hashes::{sha512, Hash, HashEngine, Hmac, HmacEngine};
use secp256k1::{PublicKey, Scalar, Secp256k1, SecretKey};
use std::sync::OnceLock;
use bitcoin::bip32::{ChainCode, ChildNumber, Xpriv};
use bip39::{Language, Mnemonic};
use bitcoin::{Network, ScriptBuf};
use bitcoin::address::Address;
static CONTEXT: OnceLock<Secp256k1<secp256k1::All>> = OnceLock::new();
'''
parts=[]
for name in names:
    token='fn '+name
    start=src.rfind('\n',0,src.index(token))+1
    body=src.index('{',src.index(token)); depth=1; end=body+1
    while depth:
        depth += (src[end]=='{') - (src[end]=='}'); end+=1
    text=src[start:end]
    # Hide unsafe raw-pointer routines, rather than export the WASM ABI.
    text=text.replace('pub unsafe extern "C" fn ', 'pub(super) unsafe fn ')
    if name in ['ctx','wipe_bytes','wipe_string','wipe_xpriv']:
        text=text.replace('fn '+name,'pub(super) fn '+name,1)
    parts.append(text)
extraction = json.dumps({'commit':commit,'source_sha256':hashlib.sha256(blob).hexdigest(),'functions':names,'changes':'imports subset; exported C functions become parent-visible Rust functions; helper visibility only; function bodies unchanged'},indent=2)+'\n'
verify_generated('extraction.json', extraction, manifest)
(root/'native/src/upstream_core.rs').write_text(imports+'\n\n'.join(parts)+'\n')
# extraction.json is a preserved reviewed input, not a regenerated trust record.
