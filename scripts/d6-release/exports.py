from pathlib import Path
import json,hashlib,runpy,shlex
R=Path(__file__).resolve().parents[2];D=R/'docs/d6-release'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
m=json.loads((D/'candidate-source-identity.json').read_text())['source_paths']
for n,h in m.items():assert sha(R/n)==h,n
identity=D/'candidate-source-identity.json'; digest=sha(identity)
mod=runpy.run_path(str(R/'scripts/release-hygiene.py'));pairs=mod['mappings'](R)
cflags=' '.join('-ffile-prefix-map='+a+'='+b for a,b in pairs)
rustflags='-C relocation-model=static '+' '.join('--remap-path-prefix='+a+'='+b for a,b in pairs)
values={'ENTROPYLAB_C_PREFIX_FLAGS':cflags,'CFLAGS_riscv32imafc_esp_espidf':'-march=rv32imafc -mabi=ilp32f -fno-pic -fno-pie '+cflags,'CARGO_TARGET_RISCV32IMAFC_ESP_ESPIDF_RUSTFLAGS':rustflags,'ENTROPYLAB_PROJECT_VER':'d6'+digest[:24]+'-r1','ENTROPYLAB_SOURCE_SHA256':digest}
for k,v in values.items():print('export '+k+'='+shlex.quote(v))
