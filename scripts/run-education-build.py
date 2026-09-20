#!/usr/bin/env python3
"""Bounded local candidate build with child status, stream and artifact bindings."""
import argparse
import datetime
import importlib.util
import json
import os
from pathlib import Path
import runpy
import shlex
import signal
import subprocess

spec = importlib.util.spec_from_file_location('identity', Path(__file__).with_name('education-successor.py'))
m = importlib.util.module_from_spec(spec)
spec.loader.exec_module(m)


def inventory(root):
    return {p.relative_to(root).as_posix(): m.sha(p.read_bytes())
            for p in sorted(root.rglob('*')) if p.is_file()}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('candidate', type=Path)
    args = parser.parse_args()
    d = args.candidate.resolve()
    m.require(d.name.startswith('education-candidate-'), 'candidate namespace required')
    source = d / 'source'
    data = m.load()
    m.validate_tree(source, data['entries'])
    m.require(not (d / 'build-status.json').exists(), 'receipt already exists; use fresh candidate')
    for p in ['fixture-firmware/build', 'fixture-firmware/runtime/target']:
        m.require(not (source / p).exists(), 'fresh build directory required')
    recipe_names = ['education-successor.py', 'build-education-successor.sh', 'run-education-build.py']
    recipe = {n: m.sha((m.ROOT / 'scripts' / n).read_bytes()) for n in recipe_names}
    m.require(not m.git('status', '--porcelain', '--', *['scripts/' + n for n in recipe_names]), 'commit recipe before build')
    digest = m.sha(m.MANIFEST.read_bytes())
    pairs = runpy.run_path(str(source / 'scripts/release-hygiene.py'))['mappings'](source)
    m.require(not any(any(c.isspace() or c in '=;\"\'\\' for c in p) for pair in pairs for p in pair), 'unsupported remap path')
    cflags = ' '.join('-ffile-prefix-map=' + a + '=' + b for a, b in pairs)
    values = {'ENTROPYLAB_C_PREFIX_FLAGS': cflags,
              'CFLAGS_riscv32imafc_esp_espidf': '-march=rv32imafc -mabi=ilp32f -fno-pic -fno-pie ' + cflags,
              'CARGO_TARGET_RISCV32IMAFC_ESP_ESPIDF_RUSTFLAGS': '-C relocation-model=static ' + ' '.join('--remap-path-prefix=' + a + '=' + b for a, b in pairs),
              'ENTROPYLAB_PROJECT_VER': 'e' + digest[:24] + '-e1',
              'ENTROPYLAB_SOURCE_SHA256': digest}
    exports = d / 'exports.sh'
    exports.write_text(''.join('export ' + k + '=' + shlex.quote(v) + '\n' for k, v in values.items()))
    managed = source / 'fixture-firmware/app/managed_components'
    inputs = inventory(managed)
    (d / 'managed-inputs.json').write_bytes(m.canonical(inputs))
    env = os.environ.copy()
    env.update(EDUCATION_SOURCE=str(source), EDUCATION_EXPORTS=str(exports), PYTHONDONTWRITEBYTECODE='1', PYTHONOPTIMIZE='0')
    command = ['bash', str(m.ROOT / 'scripts/build-education-successor.sh')]
    now = lambda: datetime.datetime.now(datetime.timezone.utc).isoformat()
    status = {'schema': 'education-local-build-v1', 'source_revision': m.REVISION,
              'source_manifest_sha256': digest, 'recipe_revision': m.git('rev-parse', 'HEAD').decode().strip(),
              'recipe_sha256': recipe, 'descriptor': values['ENTROPYLAB_PROJECT_VER'],
              'command': command, 'cwd': str(d), 'started': now(), 'status': 'running',
              'timeout_seconds': 2400, 'runner_pid': os.getpid(), 'log': str(d / 'build.log'),
              'environment': {k: env.get(k) for k in ['IDF_PATH', 'IDF_TOOLS_PATH', 'RUSTUP_HOME', 'CARGO_HOME', 'CARGO', 'RUSTC', 'RUSTDOC', 'PATH', 'CARGO_NET_OFFLINE', 'IDF_COMPONENT_CACHE_PATH', 'HOME', 'TMPDIR']},
              'exports_sha256': m.sha(exports.read_bytes()),
              'managed_input_inventory_sha256': m.sha((d / 'managed-inputs.json').read_bytes()),
              'distribution': 'HOLD', 'hardware_acceptance': 'NOT TESTED'}
    save = lambda: (d / 'build-status.json').write_bytes(m.canonical(status))
    with (d / 'build.log').open('w') as log:
        child = subprocess.Popen(command, cwd=d, env=env, stdout=log, stderr=subprocess.STDOUT, start_new_session=True)
        status['child_pid'] = child.pid
        save()
        try:
            code = child.wait(timeout=2400)
        except subprocess.TimeoutExpired:
            os.killpg(child.pid, signal.SIGTERM)
            try:
                child.wait(timeout=15)
            except subprocess.TimeoutExpired:
                os.killpg(child.pid, signal.SIGKILL)
                child.wait()
            code = 124
    status.update(child_exit_code=child.returncode, exit_code=code, finished=now(), log_sha256=m.sha((d / 'build.log').read_bytes()))
    try:
        m.validate_tree(source, data['entries'])
        m.require(inventory(managed) == inputs, 'managed inputs changed during build')
        m.require(recipe == {n: m.sha((m.ROOT / 'scripts' / n).read_bytes()) for n in recipe}, 'recipe drift')
        status['post_source_check'] = 'PASS'
    except ValueError as exc:
        status['post_source_check'] = str(exc)
        code = code or 1
    outputs = ['entropylab_fixture.bin', 'entropylab_fixture.elf', 'entropylab_fixture.map',
               'bootloader/bootloader.bin', 'partition_table/partition-table.bin',
               'flasher_args.json', 'compile_commands.json', 'project_description.json']
    build = source / 'fixture-firmware/build'
    status['artifacts'] = {n: {'sha256': m.sha((build / n).read_bytes()), 'bytes': (build / n).stat().st_size}
                           for n in outputs if (build / n).is_file()}
    if code == 0 and set(status['artifacts']) != set(outputs):
        code = 1
        status['missing_outputs'] = sorted(set(outputs) - set(status['artifacts']))
    status.update(exit_code=code, status='local-build-passed' if code == 0 else 'failed')
    save()
    print(json.dumps(status, indent=2))
    raise SystemExit(code)


if __name__ == '__main__':
    main()
