#!/usr/bin/env bash
# Candidate-only build. Does not invoke any historical accepted-source exporter.
set -euo pipefail
: "${EDUCATION_SOURCE:?fresh staged candidate source required}"
: "${EDUCATION_EXPORTS:?verified candidate export file required}"
: "${IDF_PATH:?pinned ESP-IDF required}"
: "${EDUCATION_RUN:?runner-owned private output directory required}"
# Do not execute export.sh until identity/cleanliness checks have passed.
[[ "$(/usr/bin/git -C "$IDF_PATH" rev-parse HEAD)" == b774170ff46c393eeb5e495ea37936038d3f4f4f ]]
[[ -z "$(/usr/bin/git -C "$IDF_PATH" status --porcelain --untracked-files=all --ignored)" ]]
SUBMODULE_STATUS="$(/usr/bin/git -C "$IDF_PATH" submodule status --recursive)"
if /usr/bin/grep -E '^[+U-]' <<< "$SUBMODULE_STATUS"; then exit 1; fi
/usr/bin/git -C "$IDF_PATH" submodule foreach --recursive 'test -z "$(git status --porcelain --untracked-files=all --ignored)"'
source "$IDF_PATH/export.sh"
source "$EDUCATION_EXPORTS"
unset RUSTFLAGS CARGO_ENCODED_RUSTFLAGS RUSTC_WRAPPER RUSTC_WORKSPACE_WRAPPER BASH_ENV ENV
# Bind Cargo to the compiler actually attested, not a caller override.
RUSTC="$(rustup which --toolchain nightly-2026-04-15 rustc)"
RUSTDOC="$(rustup which --toolchain nightly-2026-04-15 rustdoc)"
CARGO="$(rustup which --toolchain nightly-2026-04-15 cargo)"
export RUSTC RUSTDOC CARGO
export CC_riscv32imafc_esp_espidf=riscv32-esp-elf-gcc AR_riscv32imafc_esp_espidf=riscv32-esp-elf-ar
# Capture post-export environment and resolved executable bytes (not only versions).
python3 - <<'PY'
import hashlib, json, os, pathlib, shutil, subprocess
names = ['riscv32-esp-elf-gcc', 'riscv32-esp-elf-ar', 'riscv32-esp-elf-nm',
         'riscv32-esp-elf-readelf', 'cmake', 'ninja', 'python3', 'idf.py']
tools = {name: shutil.which(name) for name in names}
tools.update({name: os.environ[name] for name in ['RUSTC', 'RUSTDOC', 'CARGO']})
records = {}
for name, path in tools.items():
    if not path:
        raise SystemExit('missing tool: ' + name)
    p = pathlib.Path(path).resolve(strict=True)
    records[name] = {'path': str(p), 'sha256': hashlib.sha256(p.read_bytes()).hexdigest()}
records['rustc_version'] = subprocess.check_output([os.environ['RUSTC'], '-Vv'], text=True)
records['gcc_version'] = subprocess.check_output([tools['riscv32-esp-elf-gcc'], '--version'], text=True)
p = pathlib.Path(os.environ['EDUCATION_RUN']) / 'effective-environment.json'
fd = os.open(p, os.O_WRONLY | os.O_CREAT | os.O_EXCL | os.O_NOFOLLOW, 0o600)
with os.fdopen(fd, 'w') as f:
    json.dump({'environment': dict(os.environ), 'tools': records,
               'boundary': 'operator-pinned installations; system tools/libraries unpinned; not reproducibility'}, f, sort_keys=True)
PY
export CC_riscv32imafc_esp_espidf=riscv32-esp-elf-gcc AR_riscv32imafc_esp_espidf=riscv32-esp-elf-ar
R="$EDUCATION_SOURCE"; B="$R/fixture-firmware/build"
printf 'Education source identity %s; candidate descriptor %s\n' "$ENTROPYLAB_SOURCE_SHA256" "$ENTROPYLAB_PROJECT_VER"
"$RUSTC" -Vv
riscv32-esp-elf-gcc --version
idf.py --version
cmp "$R/fixture-firmware/app/sdkconfig.baseline" "$R/fixture-firmware/app/sdkconfig"
cd "$R/fixture-firmware/app"
LOCK_SHA="$(sha256sum dependencies.lock)"
mkdir -p "$R/fixture-firmware/logs"
idf.py -B "$B" -D 'SDKCONFIG_DEFAULTS=sdkconfig.defaults;rev1_3.defaults' -D "PROJECT_VER=$ENTROPYLAB_PROJECT_VER" reconfigure
python3 "$R/scripts/check-idf-remaps.py" "$B" app
cmake --build "$B" --target bootloader-configure
python3 "$R/scripts/check-idf-remaps.py" "$B/bootloader" bootloader
[[ "$(sha256sum dependencies.lock)" == "$LOCK_SHA" ]]
"$CARGO" build --offline --release --target riscv32imafc-esp-espidf -Zbuild-std=core,alloc --locked --manifest-path "$R/fixture-firmware/runtime/Cargo.toml" --target-dir "$R/fixture-firmware/runtime/target"
idf.py -B "$B" -D 'SDKCONFIG_DEFAULTS=sdkconfig.defaults;rev1_3.defaults' -D "PROJECT_VER=$ENTROPYLAB_PROJECT_VER" build
riscv32-esp-elf-readelf -h "$R/fixture-firmware/runtime/target/riscv32imafc-esp-espidf/release/libentropylab_runtime.a" > "$R/fixture-firmware/logs/archive-headers.txt"
riscv32-esp-elf-nm "$B/entropylab_fixture.elf" > "$R/fixture-firmware/logs/symbols.txt"
python3 "$R/fixture-firmware/verify.py"
python3 "$R/scripts/verify-runtime-elf.py"
python3 "$R/scripts/test-runtime-lock.py"
[[ "$(sha256sum dependencies.lock)" == "$LOCK_SHA" ]]
